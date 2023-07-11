#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
int** graphMatrix;
int*** routingTable;

struct sptNode {
	int dist;
	int parent;
} DijkstraNode;

void initialRouteTable(int node_num) {
	for(int i = node_num - 1; i >= 0; i--){
        for(int j = node_num - 1; j >= 0; j--){            
            routingTable[i][j][0] = (i == j) ? j : -1;
            routingTable[i][j][1] = (i == j) ? 0 : -999; 
        }
    }
}

int isInArray(int value, int* array, int size) {
	for (int i = 0; i < size; i++) {
		if (array[i] == value)
			return 1;
	}
	return 0;
}

void dijkstra_spt(int node_num) {
	int cur_node = 0;
	struct sptNode* spt;
	int* visited_node;

	while (cur_node < node_num) {
		spt = (struct sptNode*)malloc(sizeof(struct sptNode) * node_num);

		for (int i = 0; i < node_num; i++) {
			spt[i].dist = i == cur_node ? 0 : INT_MAX;
			spt[i].parent = i == cur_node ? i : -1;
		}

		for (int i = 0; i < node_num; i++) {
			if (graphMatrix[cur_node][i] != 0) {
				spt[i].dist = graphMatrix[cur_node][i];
				spt[i].parent = cur_node;
			}
		}

		int visited_cnt = 1, next_node = -1;
		visited_node = (int*)malloc(sizeof(int) * node_num);
		visited_node[0] = cur_node;

		while (visited_cnt < node_num) {
			int min_dist = INT_MAX;
			for (int i = 0; i < node_num; i++) {
				if (spt[i].dist > 0 && spt[i].dist < min_dist && !isInArray(i, visited_node, visited_cnt)) {
					next_node = i;
					min_dist = spt[i].dist;
				}
			}

			if (next_node == -1)
				break;

			visited_node[visited_cnt++] = next_node;
			for (int i = 0; i < node_num; i++) {
				if (graphMatrix[next_node][i] != 0 && spt[next_node].dist + graphMatrix[next_node][i] < spt[i].dist) {
					spt[i].dist = spt[next_node].dist + graphMatrix[next_node][i];
					spt[i].parent = next_node;
				}
			}
		}

		for (int i = 0; i < node_num; i++) {
			if (i != cur_node && spt[i].dist != INT_MAX) {
				int pathNode_Idx = i;
				while (spt[pathNode_Idx].parent != cur_node)
					pathNode_Idx = spt[pathNode_Idx].parent;
				routingTable[cur_node][i][0] = pathNode_Idx;
				routingTable[cur_node][i][1] = spt[i].dist;
			}
		}

		cur_node++;
		free(visited_node);
		free(spt);
	}
}


void handleMessages(const char* filename, FILE* out) {
    FILE* msgFile = fopen(filename, "r");
    if (!msgFile) {
        printf("Error opening file: %s\n", filename);
        exit(1);
    }

    char buffer[500];
    int src, dest, idx;
    char temp;

    while (fscanf(msgFile, "%d %d ", &src, &dest) == 2) {
        idx = 0;

        while ((temp = fgetc(msgFile)) != '\n' && temp != EOF) {
            buffer[idx++] = temp;
        }
        buffer[idx] = '\0';

        if (routingTable[src][dest][1] != -999) {
            fprintf(out, "from %d to %d cost %d hops ", src, dest, routingTable[src][dest][1]);
            
            do {
                fprintf(out, "%d ", src);
                src = routingTable[src][dest][0];
            } while (src != dest);

            fprintf(out, "message %s\n", buffer);
        } else {
            fprintf(out, "from %d to %d cost infinite hops unreachable message %s\n", src, dest, buffer);
        }

        if (temp == EOF)
            break;
    }

    fprintf(out, "\n");

    if (fclose(msgFile) != 0) {
        printf("Error closing file: %s\n", filename);
        exit(1);
    }
}

void printRouteTable(FILE* output_F, int node_num) {
	int nodeA, nodeB;
	for(nodeA=0; nodeA<node_num; nodeA++){
		for(nodeB=0; nodeB<node_num; nodeB++){
			if(routingTable[nodeA][nodeB][1] == -999) continue;
			fprintf(output_F, "%d %d %d\n", nodeB, routingTable[nodeA][nodeB][0], routingTable[nodeA][nodeB][1]);
		}
		fprintf(output_F, "\n");
	}
}

FILE* openFile(const char* filename, const char* mode) {
    FILE* file = fopen(filename, mode);
    if (file == NULL) {
        printf("Error: open input file %s.", filename);
        exit(1);
    }
    return file;
}

void initializeDataStructures(int node_num) {
    graphMatrix = (int**)malloc(sizeof(int*) * node_num);
    routingTable = (int***)malloc(sizeof(int**) * node_num);
    for (int i = 0; i < node_num; i++) {
        graphMatrix[i] = (int*)malloc(sizeof(int) * node_num);
        routingTable[i] = (int**)malloc(sizeof(int*) * node_num);
        for (int j = 0; j < node_num; j++) {
            graphMatrix[i][j] = 0;
            routingTable[i][j] = (int*)malloc(sizeof(int) * 2);
            routingTable[i][j][0] = -1; 
            routingTable[i][j][1] = -999;
        }
    }
}

void freeDataStructures(int node_num) {
    int i, j;
    for (i = 0; i < node_num; i++) {
        for (j = 0; j < node_num; j++) {
            free(routingTable[i][j]);
        }
        free(graphMatrix[i]);
        free(routingTable[i]);
    }
    free(graphMatrix);
    free(routingTable);
}

void applyChangesAndRecalculate(const char* messageFile, FILE* changeFile, FILE* outputFile, int node_num) {
    int u, v, dist;
    while (1) {
        if (fscanf(changeFile, "%d %d %d", &u, &v, &dist) == EOF)
            break;
        if (dist < 0)
            dist = 0;
        graphMatrix[u][v] = dist;
        graphMatrix[v][u] = dist;

        initialRouteTable(node_num);
        dijkstra_spt(node_num);
        printRouteTable(outputFile, node_num);
        handleMessages(messageFile, outputFile);
    }
}


int main(int argc, char* argv[]) {

    int node_num, u, v, dist;

    if (argc != 4) {
        printf("usage: distvec topologyfile messagesfile changesfile");
        exit(1);
    }

    FILE* topologyFile = openFile(argv[1], "r");
    FILE* messagesFile = openFile(argv[2], "r");
    fclose(messagesFile); 
    FILE* changesFile = openFile(argv[3], "r");

    fscanf(topologyFile, "%d", &node_num);

    initializeDataStructures(node_num);

    while (1) {
        if (fscanf(topologyFile, "%d %d %d", &u, &v, &dist) == EOF)
            break;
        
        graphMatrix[u][v] = dist;
        graphMatrix[v][u] = dist;
    }

    initialRouteTable(node_num);
    dijkstra_spt(node_num);

    FILE* outputFile = openFile("output_ls.txt", "w");
    printRouteTable(outputFile, node_num);
    handleMessages(argv[2], outputFile);

    applyChangesAndRecalculate(argv[2], changesFile, outputFile, node_num);

    freeDataStructures(node_num);

    fclose(topologyFile);
    fclose(changesFile);
    fclose(outputFile);

    printf("Complete. Output file written to output_ls.txt\n");

    return 0;
}