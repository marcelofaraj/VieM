/******************************************************************************
 * viem.cpp
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

#include <argtable2.h>
#include <iostream>
#include <math.h>
#include <regex.h>
#include <sstream>
#include <stdio.h>
#include <string.h> 

#include "balance_configuration.h"
#include "data_structure/graph_access.h"
#include "data_structure/matrix/normal_matrix.h"
#include "data_structure/matrix/online_distance_matrix.h"
#include "graph_io.h"
#include "macros_assertions.h"
#include "parse_parameters.h"
#include "partition/graph_partitioner.h"
#include "partition/partition_config.h"
#include "partition/uncoarsening/refinement/cycle_improvements/cycle_refinement.h"
#include "mapping/mapping_algorithms.h"
#include "quality_metrics.h"
#include "random_functions.h"
#include "timer.h"

int main(int argn, char **argv) {

        PartitionConfig config;
        std::string graph_filename;

        bool is_graph_weighted = false;
        bool suppress_output   = false;
        bool recursive         = false;
       
        int ret_code = parse_parameters(argn, argv, 
                                        config, 
                                        graph_filename, 
                                        is_graph_weighted, 
                                        suppress_output, recursive); 

        if(ret_code) {
                return 0;
        }

        std::streambuf* backup = std::cout.rdbuf();
        std::ofstream ofs;
        ofs.open("/dev/null");
        if(suppress_output) {
                std::cout.rdbuf(ofs.rdbuf()); 
        }

        config.LogDump(stdout);
        graph_access G;     

        timer_x t;
        graph_io::readGraphWeighted(G, graph_filename);
        std::cout << "io time: " << t.elapsed()  << std::endl;
       
        G.set_partition_count(config.k); 

        balance_configuration bc;
        bc.configurate_balance( config, G);

        srand(config.seed);
        random_functions::setSeed(config.seed);

        // ***************************** perform mapping ***************************************       
        t.restart();
 
        srand(config.seed);
        random_functions::setSeed(config.seed);

        std::cout <<  "graph has " <<  G.number_of_nodes() <<  " nodes and " <<  G.number_of_edges() <<  " edges"  << std::endl;
        // ***************************** perform partitioning ***************************************       
        t.restart();
        graph_partitioner partitioner;

        std::cout <<  "performing partitioning! " << std::endl;

        partitioner.perform_partitioning(config, G);

        std::cout << "partitioning took " << t.elapsed()  << std::endl;

        graph_access C;
        complete_boundary boundary(&G);
        boundary.build();
        boundary.getUnderlyingQuotientGraph(C);

        forall_nodes(C, node) {
                C.setNodeWeight(node, 1);
        } endfor
        std::cout <<  "model has " << C.number_of_nodes() << " nodes, " << C.number_of_edges() <<  " edges "  << std::endl;
        std::cout <<  "writing model of computation and communication to disk"  << std::endl;
        
        std::stringstream filename;
        if(!config.filename_output.compare("")) {
                filename << "model.graph";
        } else {
                filename << config.filename_output;
        }

        graph_io::writeGraphWeighted( C, filename.str());
         
        ofs.close();
        std::cout.rdbuf(backup);
}
