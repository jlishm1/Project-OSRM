/*
    open source routing machine
    Copyright (C) Dennis Luxen, others 2010

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU AFFERO General Public License as published by
the Free Software Foundation; either version 3 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
or see http://www.gnu.org/licenses/agpl.txt.
 */

#ifndef NODEINFORMATIONHELPDESK_H_
#define NODEINFORMATIONHELPDESK_H_

#include "NodeCoords.h"
#include "PhantomNodes.h"
#include "StaticRTree.h"
#include "../Contractor/EdgeBasedGraphFactory.h"
#include "../typedefs.h"

#include <boost/assert.hpp>
#include <boost/noncopyable.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

typedef EdgeBasedGraphFactory::EdgeBasedNode RTreeLeaf;

class NodeInformationHelpDesk : boost::noncopyable{
public:
    NodeInformationHelpDesk(
        const std::string & ramIndexInput,
        const std::string & fileIndexInput,
        const std::string & nodes_filename,
        const std::string & edges_filename,
        const unsigned number_of_nodes,
        const unsigned check_sum
        ) : number_of_nodes(number_of_nodes), check_sum(check_sum)
    {
        read_only_rtree = new StaticRTree<RTreeLeaf>(
            ramIndexInput,
            fileIndexInput
        );
        BOOST_ASSERT_MSG(
            0 == coordinateVector.size(),
            "Coordinate vector not empty"
        );

        LoadNodesAndEdges(nodes_filename, edges_filename);
    }

    //Todo: Shared memory mechanism
	~NodeInformationHelpDesk() {
		delete read_only_rtree;
	}

	inline int getLatitudeOfNode(const unsigned id) const {
	    const NodeID node = origEdgeData_viaNode.at(id);
	    return coordinateVector.at(node).lat;
	}

	inline int getLongitudeOfNode(const unsigned id) const {
        const NodeID node = origEdgeData_viaNode.at(id);
	    return coordinateVector.at(node).lon;
	}

	inline unsigned getNameIndexFromEdgeID(const unsigned id) const {
	    return origEdgeData_nameID.at(id);
	}

    inline TurnInstruction getTurnInstructionFromEdgeID(const unsigned id) const {
        return origEdgeData_turnInstruction.at(id);
    }

    inline NodeID getNumberOfNodes() const {
        return number_of_nodes;
    }

	inline NodeID getNumberOfNodes2() const {
        return coordinateVector.size();
    }

    inline bool FindNearestNodeCoordForLatLon(
            const _Coordinate& input_coordinate,
            _Coordinate& result,
            const unsigned zoom_level = 18
    ) const {
        PhantomNode resulting_phantom_node;
        bool foundNode = FindPhantomNodeForCoordinate(
            input_coordinate,
            resulting_phantom_node, zoom_level
        );
        result = resulting_phantom_node.location;
        return foundNode;
    }

    inline bool FindPhantomNodeForCoordinate(
            const _Coordinate & input_coordinate,
            PhantomNode & resulting_phantom_node,
            const unsigned zoom_level
    ) const {
        return read_only_rtree->FindPhantomNodeForCoordinate(
                input_coordinate,
                resulting_phantom_node,
                zoom_level
        );
    }

	inline unsigned GetCheckSum() const {
	    return check_sum;
	}

private:
    void LoadNodesAndEdges(
        const std::string & nodes_file,
        const std::string & edges_file
    ) {
    std::ifstream nodes_input_stream(nodes_file.c_str(), std::ios::binary);
    if(!nodes_input_stream) { ERR(nodes_file <<  " not found"); }
    std::ifstream edges_input_stream(edges_file.c_str(), std::ios::binary);
    if(!edges_input_stream) { ERR(edges_file <<  " not found"); }

        DEBUG("Loading node data");
        NodeInfo b;
        while(!nodes_input_stream.eof()) {
            nodes_input_stream.read((char *)&b, sizeof(NodeInfo));
            coordinateVector.push_back(_Coordinate(b.lat, b.lon));
        }
        std::vector<_Coordinate>(coordinateVector).swap(coordinateVector);
        nodes_input_stream.close();

        DEBUG("Loading edge data");
        unsigned numberOfOrigEdges(0);
        edges_input_stream.read((char*)&numberOfOrigEdges, sizeof(unsigned));
        origEdgeData_viaNode.resize(numberOfOrigEdges);
        origEdgeData_nameID.resize(numberOfOrigEdges);
        origEdgeData_turnInstruction.resize(numberOfOrigEdges);

        OriginalEdgeData deserialized_originalEdgeData;
        for(unsigned i = 0; i < numberOfOrigEdges; ++i) {
            edges_input_stream.read(
                (char*)&(deserialized_originalEdgeData),
                sizeof(OriginalEdgeData)
            );
            origEdgeData_viaNode[i] = deserialized_originalEdgeData.viaNode;
            origEdgeData_nameID[i]  = deserialized_originalEdgeData.nameID;
            origEdgeData_turnInstruction[i] = deserialized_originalEdgeData.turnInstruction;
        }
        edges_input_stream.close();
        DEBUG("Loaded " << numberOfOrigEdges << " orig edges");
        DEBUG("Opening NN indices");
    }

	std::vector<_Coordinate> coordinateVector;
	std::vector<NodeID> origEdgeData_viaNode;
	std::vector<unsigned> origEdgeData_nameID;
	std::vector<TurnInstruction> origEdgeData_turnInstruction;

	StaticRTree<EdgeBasedGraphFactory::EdgeBasedNode> * read_only_rtree;
	const unsigned number_of_nodes;
	const unsigned check_sum;
};

#endif /*NODEINFORMATIONHELPDESK_H_*/
