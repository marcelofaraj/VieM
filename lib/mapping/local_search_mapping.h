/******************************************************************************
 * local_search_mapping.h
 *
 * Source of VieM -- Vienna Mapping and Sparse Quadratic Assignment 
 ******************************************************************************
 * Copyright (C) 2017 Christian Schulz 
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************************/

#ifndef LOCAL_SEARCH_MAPPING_CCR5FJN
#define LOCAL_SEARCH_MAPPING_CCR5FJN

#include "partition_config.h"
#include "data_structure/graph_access.h"
#include "data_structure/matrix/matrix.h"
#include "tools/timer.h"
#include "tools/quality_metrics.h"

class local_search_mapping {
public:
        local_search_mapping();
        virtual ~local_search_mapping();
           
        template < typename search_space > 
        void perform_local_search( PartitionConfig & config, graph_access & C, matrix & D, std::vector< NodeID > & perm_rank);

private:
        bool perform_single_swap(graph_access & C, matrix & D, std::vector< NodeID > & perm_rank, NodeID swap_lhs, NodeID swap_rhs);
        void update_node_contribution( graph_access & C, matrix & D, std::vector< NodeID > & perm_rank, NodeID swap_lhs, NodeID swap_rhs);

        // Data Members
        std::vector< NodeID > node_contribution;
        NodeWeight total_volume;
        quality_metrics qm;
};

// input a valid initial mapping
// output a valid hopefully better mapping
template < typename search_space > 
void local_search_mapping::perform_local_search( PartitionConfig & config, graph_access & C, matrix & D, std::vector< NodeID > & perm_rank) {
        timer_x t; t.restart();

        //compute total metric
        total_volume = 0;
        node_contribution.resize(C.number_of_nodes(), 0);
        forall_nodes(C, node) {
                forall_out_edges(C, e, node) {
                        NodeID target              = C.getEdgeTarget(e);
                        NodeWeight comm_vol        = C.getEdgeWeight(e);
                        NodeID perm_rank_node      = perm_rank[node];
                        NodeID perm_rank_target    = perm_rank[target];
                        NodeWeight cur_vol         = comm_vol*D.get_xy(perm_rank_node, perm_rank_target);
                        node_contribution[ node ] += cur_vol;
                } endfor
                total_volume += node_contribution[node]; 
        } endfor
        std::cout <<  "J(C,D,Pi) = " <<  total_volume << std::endl;
        //std::cout <<  "Diameter " << qm.diameter(C) << std::endl;

        search_space fss(config, C.number_of_nodes());
	fss.set_graph_ref( &C);

        while ( !fss.done() ) {
                std::pair< NodeID, NodeID > cur_pair = fss.nextPair();

                NodeID swap_lhs = cur_pair.first;
                NodeID swap_rhs = cur_pair.second;

                if( D.get_xy(perm_rank[swap_lhs], perm_rank[swap_rhs]) == config.distances[0] ) {
                        fss.commit_status(false);
                        continue; // skipping swaps inside nodes 
                }
                if(!perform_single_swap( C, D, perm_rank, swap_lhs, swap_rhs)) {
			fss.commit_status(false);
                } else {
			fss.commit_status(true);
		}
        }

        if( total_volume != qm.total_qap(C, D, perm_rank)) {
                std::cout <<  "objective function mismatch"  << std::endl;
                exit(0);
        }
}


#endif /* end of include guard: LOCAL_SEARCH_MAPPING_CCR5FJN */
