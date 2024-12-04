#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>

using namespace std;

struct Cube {
    int x, y, z; // Esquina menor
    int size;
};

struct OctreeNode {
    enum NodeState {EMPTY,FULL,MIXED};
    NodeState state;
    OctreeNode* children[8];
    bool hasChildren;

    OctreeNode(NodeState initialState = EMPTY) : state(initialState), hasChildren(false) {
        for (int i = 0; i < 8; ++i) {
            children[i] = nullptr;
        }
    }

    ~OctreeNode() {
        for (int i = 0; i < 8; ++i) {
            if (children[i]) {
                delete children[i];
            }
        }
    }

    void subdivide() {
        if (!hasChildren) {
            for (int i = 0; i < 8; ++i) {
                children[i] = new OctreeNode(EMPTY);
            }
            hasChildren = true;
            state = MIXED;
        }
    }

    void combine() {
        if (hasChildren) {
            NodeState childState = children[0]->state;

            if(childState == MIXED) return;

            for (int i = 1; i < 8; ++i) {
                if (children[i]->state != childState) {
                    return;
                }
            }

            for (int i = 0; i < 8; ++i) {
                delete children[i];
                children[i] = nullptr;
            }
            hasChildren = false;
            state = childState;
        }
    }

    void insert(int level, int maxLevel, int x, int y, int z) {
        if(state == FULL) return;

        if (level == maxLevel) {
            state = FULL;
            return;
        }

        subdivide(); // Subdividir (si es necesario)

        int bitShift = maxLevel - level - 1;
        int xBit = (x >> bitShift) & 1;
        int yBit = (y >> bitShift) & 1;
        int zBit = (z >> bitShift) & 1;

        int childIndex = (zBit << 2) | (yBit << 1) | xBit;

        children[childIndex]->insert(level + 1, maxLevel, x, y, z);
        combine(); // Combinar (si es posible)
    }

    bool getFull(int level, int maxLevel, int x, int y, int z) {
        if (level == maxLevel) {
            return state == FULL;
        }

        if (!hasChildren) {
            return state == FULL;
        }

        int bitShift = maxLevel - level - 1;
        int xBit = (x >> bitShift) & 1;
        int yBit = (y >> bitShift) & 1;
        int zBit = (z >> bitShift) & 1;

        int childIndex = (zBit << 2) | (yBit << 1) | xBit;

        return children[childIndex]->getFull(level + 1, maxLevel, x, y, z);
    }

    void remove(int level, int maxLevel, int x, int y, int z) {
        if (state == EMPTY) return;

        if (level == maxLevel) {
            state = EMPTY;
            return;
        }

        if (!hasChildren) {
            if (state == FULL) {
                for (int i = 0; i < 8; ++i) {
                    children[i] = new OctreeNode(FULL);
                }
                hasChildren = true;
                state = MIXED;
            }
            else {
                return;
            }
        };

        int bitShift = maxLevel - level - 1;
        int xBit = (x >> bitShift) & 1;
        int yBit = (y >> bitShift) & 1;
        int zBit = (z >> bitShift) & 1;

        int childIndex = (zBit << 2) | (yBit << 1) | xBit;

        if (children[childIndex]) {
            children[childIndex]->remove(level + 1, maxLevel, x, y, z);
            combine();
        }
    }
};

class Octree {
private:
    OctreeNode root;
    int maxDepth;

    // Método recursivo para recorrer el Octree y generar el vector de cubos
    void auxCubeVector(OctreeNode* node, int x, int y, int z, int size, vector<Cube>& cubes) {
        if (!node) return;

        if (node->state == OctreeNode::FULL) {
            cubes.push_back({x, y, z, size});
            return;
        }

        if (node->hasChildren) {
            int childSize = size / 2;

            int dx[] = {0, childSize, 0, childSize, 0, childSize, 0, childSize};
            int dy[] = {0, 0, childSize, childSize, 0, 0, childSize, childSize};
            int dz[] = {0, 0, 0, 0, childSize, childSize, childSize, childSize};

            for (int i = 0; i < 8; ++i) {
                if (node->children[i]) {
                    auxCubeVector(node->children[i], x + dx[i], y + dy[i], z + dz[i], childSize, cubes);
                }
            }
        }
    }

    // Método para exportar un nodo a formato JSON
    string auxExport(OctreeNode* node, int size) {
        ostringstream json;
        json << "{";
        json << "\"size\":" << size << ",";
        json << "\"state\":\"";
        if (node->state == OctreeNode::EMPTY) json << "EMPTY";
        else if (node->state == OctreeNode::FULL) json << "FULL";
        else json << "MIXED";
        json << "\"";
        if (node->hasChildren) {
            json << ",\"children\":[";
            for (int i = 0; i < 8; ++i) {
                if (node->children[i]) {
                    json << auxExport(node->children[i], size / 2);
                    if (i < 7) json << ",";
                }
            }
            json << "]";
        } else {
            json << ",\"children\":[]";
        }
        json << "}";
        return json.str();
    }

public:
    Octree(int maxDepth = 5) : maxDepth(maxDepth) {}

    void insert(int x, int y, int z) {
        root.insert(0, maxDepth, x, y, z);
    }

    bool getFull(int x, int y, int z) {
        return root.getFull(0, maxDepth, x, y, z);
    }

    void remove(int x, int y, int z) {
        root.remove(0, maxDepth, x, y, z);
    }

    void exportToFile() {
        ofstream file("oct.txt");
        if (file.is_open()) {
            file << auxExport(&root, 1 << maxDepth);
            file.close();
            cout << "El Octree ha sido exportado a 'oct.txt'." << endl;
        } else {
            cerr << "Error al abrir el archivo 'oct.txt' para escritura." << endl;
        }
    }

    vector<Cube> toCubeVector() {
        vector<Cube> cubes;
        auxCubeVector(&root, 0, 0, 0, 1 << maxDepth, cubes); // El tamaño inicial es 2^maxDepth
        return cubes;
    }

    void exportCubesToFile(const string& filename) {
        ofstream file(filename);
        if (!file.is_open()) {
            cerr << "Error al abrir el archivo " << filename << " para escritura." << endl;
            return;
        }

        vector<Cube> cubes = toCubeVector();
        for (const auto& cube : cubes) {
            file << cube.x << " " << cube.y << " " << cube.z << " " << cube.size << "\n";
        }

        file.close();
        cout << "Informacion de los cubos exportada a " << filename << endl;
    }
};


int main() {
    // Profundidad máxima de 5, rango [0, 31] en cada eje
    int maxDepth = 5;
    int MaxNode = (1<<maxDepth)-1;
    Octree octree(maxDepth);



    //centro y radio de la esfera
    int cx = 16, cy = 16, cz = 16;
    int radius = 14;

    for (int x = 0; x <= MaxNode; x++) {
        for (int y = 0; y <= MaxNode; y++) {
            for (int z = 0; z <= MaxNode; z++) {
                // Verificar si el punto (x, y, z) está dentro de la esfera
                int dx = x - cx;
                int dy = y - cy;
                int dz = z - cz;
                if (dx * dx + dy * dy + dz * dz <= radius * radius) {
                    octree.insert(x, y, z);
                }
            }
        }
    }

    for (int x = 0; x <= MaxNode; x++) {
        for (int y = 0; y <= MaxNode; y++) {
            for (int z = 0; z <= MaxNode; z++) {
                if (!(15 < z && z < 20)) {
                    octree.remove(x, y, z);
                }
            }
        }
    }





//
//    for (int x = 0; x <= MaxNode; x++) {
//        for (int y = 0; y <= MaxNode; y++) {
//            for (int z = 0; z <= MaxNode; z++) {
//                if (x > 0){
//                    octree.insert(x, y, z);
//                }
//            }
//        }
//    }
//
//    for (int x = 0; x <= MaxNode; x++) {
//        for (int y = 0; y <= MaxNode; y++) {
//            for (int z = 0; z <= MaxNode; z++) {
//                cout << octree.getFull(x, y, z) << " ";
//            }
//            cout << endl;
//        }
//        cout << endl;
//    }
//    //Imprimir el octree
//    vector<Cube> cubes = octree.toCubeVector();

    // Mostrar los cubos
//    for (const auto& cube : cubes) {
//        cout << "Cube at (" << cube.x << ", " << cube.y << ", " << cube.z << ") with size " << cube.size << "\n";
//    }

    vector<Cube> cubes = octree.toCubeVector();
    for (const auto& cube : cubes) {
        cout << "Cube at (" << cube.x << ", " << cube.y << ", " << cube.z << ") with size " << cube.size << "\n";
    }


    // Exportamos el octree a un archivo
    //octree.exportToFile();
    octree.exportCubesToFile("data.txt");

    return 0;
}
