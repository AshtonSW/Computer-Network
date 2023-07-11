#include <stdio.h>
#include <stdlib.h>

int topo_u, topo_v, topo_cost, node_num;
int cng_u, cng_v, cng_cost; 
int src, dest;
char msg[1000];
char *msg_line;

void createRoutingTable(int ***routingTable){
    int **routingTable_memory_block = (int**)malloc(sizeof(int*) * node_num * node_num);
    int *data_memory_block = (int*)malloc(sizeof(int) * 2 * node_num * node_num);

    if(routingTable_memory_block == NULL || data_memory_block == NULL) {
        fprintf(stderr, "Error: Cannot allocate memory for the routing table.\n");
        exit(1);
    }

    for(int i=0; i<node_num; i++){
        routingTable[i] = routingTable_memory_block + i * node_num;
        for(int j=0; j<node_num; j++){
            routingTable[i][j] = data_memory_block + (i * node_num + j) * 2;
        }
    }
}

void init_routingTable(int ***routingTable){
    for(int i = node_num - 1; i >= 0; i--){
        for(int j = node_num - 1; j >= 0; j--){            
            routingTable[i][j][0] = (i == j) ? j : -1; 
            routingTable[i][j][1] = (i == j) ? 0 : -999; 
        }
    }
}

void load_direct_neighbors(FILE *fp, int ***routingTable){
    char line[256];
    while(fgets(line, sizeof(line), fp)){
        if(sscanf(line, "%d%d%d", &topo_u, &topo_v, &topo_cost) == 3){
            routingTable[topo_u][topo_v][0] = topo_v;
            routingTable[topo_u][topo_v][1] = topo_cost;
            routingTable[topo_v][topo_u][0] = topo_u;
            routingTable[topo_v][topo_u][1] = topo_cost;
        }
    }
}

void update_routing_table(int ***routingTable){
    int updated;
    int currentNode, nextNode, targetNode;
    do {
        updated = 0;
        for(currentNode = 0; currentNode < node_num; currentNode++){
            for(nextNode = 0; nextNode < node_num; nextNode++){
                if(nextNode == routingTable[currentNode][nextNode][0] && routingTable[currentNode][nextNode][1] != 0){ 
                    for(targetNode = 0; targetNode < node_num; targetNode++){ 
                        if(routingTable[currentNode][targetNode][1] == -999 || 
                          (routingTable[currentNode][nextNode][1] + routingTable[nextNode][targetNode][1] < routingTable[currentNode][targetNode][1]) ||
                          (routingTable[currentNode][nextNode][1] + routingTable[nextNode][targetNode][1] == routingTable[currentNode][targetNode][1] && nextNode < routingTable[currentNode][targetNode][0])){ 
                            if(routingTable[currentNode][nextNode][1] + routingTable[nextNode][targetNode][1] < 0){ 
                                continue; 
                            }
                            routingTable[currentNode][targetNode][0] = nextNode;
                            routingTable[currentNode][targetNode][1] = routingTable[currentNode][nextNode][1] + routingTable[nextNode][targetNode][1];
                            updated = 1; 
                        }
                    }
                }
            }
        }
    } 
    while(updated);
}

void update_table_entry(int ***routingTable, int u, int v, int cost) {
    routingTable[u][v][0] = v;
    routingTable[u][v][1] = cost;
    routingTable[v][u][0] = u;
    routingTable[v][u][1] = cost;
}

void update_path_cost(int ***routingTable){
    update_table_entry(routingTable, cng_u, cng_v, cng_cost);
}

void print_routing_table(FILE *out, int ***routingTable){
	int nodeA, nodeB;
	for(nodeA=0; nodeA<node_num; nodeA++){
		for(nodeB=0; nodeB<node_num; nodeB++){
			if(routingTable[nodeA][nodeB][1] == -999) continue;
			fprintf(out, "%d %d %d\n", nodeB, routingTable[nodeA][nodeB][0], routingTable[nodeA][nodeB][1]);
		}
		fprintf(out, "\n");
	}
}

void print_msg_path(FILE *fp, FILE *out, int ***routingTable){
    while(true){
        int result = fscanf(fp, "%d%d", &src, &dest);
        if(result == EOF) break;
        
        msg_line = fgets(msg, 1000, fp);
        if(msg_line == NULL) break;
        
        if(routingTable[src][dest][1] != -999){
            fprintf(out, "from %d to %d cost %d hops ", src, dest, routingTable[src][dest][1]);
            
            int temp_src = src;
            do {
                fprintf(out, "%d ", temp_src);
                temp_src = routingTable[temp_src][dest][0];
            } while(temp_src != dest);

            fprintf(out, "message%s", msg_line);
        }
        else if(routingTable[src][dest][1] == -999){
            fprintf(out, "from %d to %d cost infinite hops unreachable message%s", src, dest, msg_line);
        }
    }
    fprintf(out, "\n"); 
    fclose(fp);
}

FILE* open_file(char* filename, const char* mode){
    FILE* file = fopen(filename, mode);
    if (file == NULL) {
        fprintf(stderr, "Error: cannot open file %s.\n", filename);
        exit(1);
    }
    return file;
}

int main(int argc, char *argv[]) {
	
	if(argc != 4) {
	  fprintf(stderr, "usage: distvec topologyfile messagesfile changesfile\n");
	  exit(1);
	}
	
	FILE *topologyFile = open_file(argv[1], "r");
    FILE *msgFile = open_file(argv[2], "r");
    FILE *chFile = open_file(argv[3], "r");

	fscanf(topologyFile, "%d", &node_num);
	int ***routingTable;
	routingTable = (int***)malloc(sizeof(int**)*node_num); 
	createRoutingTable(routingTable); 
	init_routingTable(routingTable); 
	load_direct_neighbors(topologyFile, routingTable); 
	update_routing_table(routingTable); 

	FILE *out;
	out = fopen("output_dv.txt", "wt");
	if (out == NULL) {
		fprintf(stderr, "Error: open output file. \n");
		exit(1); 
    }
	print_routing_table(out, routingTable);
	print_msg_path(msgFile, out, routingTable);


	while(1){
		init_routingTable(routingTable); 
		topologyFile = fopen(argv[1], "r");
		if (topologyFile == NULL) {
			fprintf(stderr, "Error: open input file.\n");
			exit(1); 
        } 
        
        fscanf(topologyFile, "%d", &node_num);
			
		while(!feof(topologyFile)){ 
			fscanf(topologyFile, "%d%d%d", &topo_u, &topo_v, &topo_cost);
			routingTable[topo_u][topo_v][0] = topo_v;
			routingTable[topo_u][topo_v][1] = topo_cost;
			routingTable[topo_v][topo_u][0] = topo_u;
			routingTable[topo_v][topo_u][1] = topo_cost;
		} 
        
        fclose(topologyFile);

		fscanf(chFile, "%d%d%d", &cng_u, &cng_v, &cng_cost);
		if(feof(chFile)) break;
		update_path_cost(routingTable);
		update_routing_table(routingTable); 
		print_routing_table(out, routingTable); 

		msgFile = fopen(argv[2], "r");
		if (msgFile == NULL) {
		fprintf(stderr, "Error: open input file.\n");
		exit(1); 
        }
        int src, dest;
        while(fscanf(msgFile, "%d%d", &src, &dest) != EOF){
            char *msg_line = fgets(msg, 1000, msgFile);
            if(msg_line == NULL) break;

            int cost = routingTable[src][dest][1];
            if(cost != -999){
                fprintf(out, "from %d to %d cost %d hops ", src, dest, cost);

                for(int current = src; current != dest; current = routingTable[current][dest][0]){
                fprintf(out, "%d ", current);
                } 

                fprintf(out, "message%s", msg_line);
            } else {
                fprintf(out, "from %d to %d cost infinite hops unreachable message%s", src, dest, msg_line);
            }
        } 
        fprintf(out, "\n");     
    }

	printf("Complete. Output file written to output_dv.txt\n");
	fclose(chFile);
	fclose(out);
	return 0;
}

