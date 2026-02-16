# DEG: Dynamic Edge Navigation Graph

## Project Overview

This is a C++ implementation of **DEG (Dynamic Edge Navigation Graph)**, a research project for efficient hybrid vector search. The project implements a novel graph-based index structure that supports queries with dynamic weight parameters (alpha) combining two vector spaces (embedding and spatial/location vectors).

### Key Features

- **Hybrid Vector Query Support**: Calculates similarity using weighted sum of embedding distance and spatial distance
- **Dynamic Alpha Support**: Efficiently handles queries with varying alpha parameters (weight between embedding and spatial similarity)
- **Multiple Index Structures**: Implements HNSW, NSG, NSW, R-Tree, and the proposed DEG algorithm
- **Multi-threading**: Uses OpenMP for parallel processing
- **SIMD Optimization**: Uses AVX2 intrinsics for fast distance computation

## Technology Stack

| Component | Technology |
|-----------|------------|
| Language | C++14 |
| Build System | CMake 2.8+ |
| Parallel Processing | OpenMP |
| External Libraries | Boost 1.55+ |
| Compiler | GCC 4.9+ |

## Project Structure

```
DEG/
├── CMakeLists.txt          # Main CMake configuration
├── README.md               # Project documentation
├── AGENTS.md              # This file
├── include/               # Header files
│   ├── index.h            # Core index data structures (NNDescent, HNSW, DEG, etc.)
│   ├── builder.h          # Index builder interface
│   ├── component.h        # Component class hierarchy for modular operations
│   ├── distance.h         # Distance computation with AVX2 SIMD
│   ├── parameters.h       # Parameter management class
│   ├── policy.h           # Type enums for algorithms
│   ├── rtree.h            # R-Tree spatial index implementation
│   ├── set_para.h         # Dataset-specific parameter configuration
│   ├── util.h             # Utility functions (random generation)
│   └── CommonDataStructure.h  # Array wrapper template class
├── src/                   # Source files
│   ├── builder.cpp        # IndexBuilder implementation
│   ├── component_*.cpp    # Component implementations (init, search, prune, etc.)
│   ├── nndescent.cpp      # NN-Descent algorithm
│   └── rtree.cpp          # R-Tree implementation
├── test/                  # Test/executable code
│   └── main.cpp           # Main entry point for experiments
└── build/                 # Build output directory
```

## Architecture

### Core Components

1. **Index Class** (`index.h`): Central data structure containing:
   - Base data (embedding vectors, location vectors)
   - Query data and ground truth
   - Graph structures (HNSW nodes, DEG nodes, etc.)
   - Distance computation objects
   - Parameters

2. **IndexBuilder Class** (`builder.h`): Builder pattern for:
   - Loading datasets
   - Initializing index structures
   - Building and saving graphs
   - Performing searches
   - Performance evaluation

3. **Component Hierarchy** (`component.h`): Modular design with components for:
   - `ComponentLoad`: Data loading
   - `ComponentInit*`: Index initialization (HNSW, DEG, RTree, etc.)
   - `ComponentSearchRoute*`: Search routing strategies
   - `ComponentPrune*`: Graph pruning algorithms
   - `ComponentRefine*`: Graph refinement (NN-Descent, NSG)

### Supported Algorithms

| Algorithm | Type | Description |
|-----------|------|-------------|
| HNSW | Baseline | Hierarchical Navigable Small World |
| baseline1 | Baseline | HNSW with fixed alpha=0.5 |
| baseline2 | Baseline | Dual HNSW indices (alpha=0 and alpha=1) |
| baseline3 | Baseline | R-Tree + HNSW hybrid |
| baseline4 | Baseline | Multi-level HNSW variant |
| DEG | Proposed | Dynamic Edge Navigation Graph |

### Data Format

The project uses standard vector file formats:
- **`.fvecs`**: Float vectors (base embeddings, query embeddings, locations)
- **`.ivecs`**: Integer vectors (ground truth labels)

Format specification: http://yael.gforge.inria.fr/file_format.html

### Supported Datasets

The code includes hardcoded paths for these datasets in `include/set_para.h`:
- `openimage` - Open Images dataset
- `sg-ins` - Singapore Instagram dataset
- `howto100m` - HowTo100M dataset
- `cc3m` - Conceptual Captions 3M
- `Twitter10M` - Twitter dataset

**Note**: Dataset paths are hardcoded to `/mnt/hdd/yinziqi/yinziqi/graphann-tkq/dataset/` in `test/main.cpp`.

## Build Instructions

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake libboost-all-dev

# Ensure GCC version >= 4.9
gcc --version
```

### Build Commands

```bash
# Create build directory
mkdir -p build && cd build/

# Configure
cmake ..

# Build with parallel jobs
make -j

# The main executable will be at: build/test/main
```

### CMake Options

The `CMakeLists.txt` sets these important flags:
- `-std=c++14`: C++14 standard
- `-O2`: Optimization level 2
- `-march=native`: Optimize for current CPU
- `-Wall`: Enable all warnings
- `-DINFO`: Enable info logging

## Usage

### Command Line Interface

```bash
./main <algorithm> <dataset> <alpha> <max_spatial_distance> <max_emb_distance> <exc_type>
```

Arguments:
- `algorithm`: One of `hnsw`, `baseline1`, `baseline2`, `baseline3`, `baseline4`, `deg`
- `dataset`: One of `openimage`, `sg-ins`, `howto100m`, `cc3m`, `Twitter10M`
- `alpha`: Weight parameter (0.0 to 1.0)
- `max_spatial_distance`: Maximum spatial distance normalization factor
- `max_emb_distance`: Maximum embedding distance normalization factor
- `exc_type`: `build` or `search`

### Example Commands

```bash
# Build DEG index
cd build/test/
./main deg openimage 0.5 1 1 build

# Search with DEG index
./main deg openimage 0.5 1 1 search
```

### Output Files

Index files are saved to `/mnt/hdd/yinziqi/yinziqi/graphann-tkq/saved_index/` (hardcoded):
- HNSW: `{alg}_{dataset}.index`
- DEG: `{alg}_{dataset}.index`
- BS4: `{alg}_{dataset}.index_subindex_0` to `_4`
- R-Tree: Custom binary format

## Code Style Guidelines

### Naming Conventions

- **Classes**: PascalCase (e.g., `IndexBuilder`, `ComponentInitHNSW`)
- **Methods**: PascalCase for public methods (e.g., `InitInner()`, `LoadInner()`)
- **Private members**: snake_case with underscore suffix (e.g., `final_index_`, `max_level_`)
- **Constants**: UPPER_CASE (e.g., `MAXNODES`, `RTREEMAXNODES`)
- **Namespaces**: `stkq` (all code is within this namespace)

### Comments

- Comments are primarily in **Chinese** in the existing codebase
- Use `//` for single-line comments
- Key algorithms have inline comments explaining the logic

### Code Patterns

1. **Component Pattern**: Operations are encapsulated in component classes inheriting from base `Component`
2. **Builder Pattern**: `IndexBuilder` provides fluent interface (method chaining)
3. **Type Safety**: Uses `TYPE` enum for algorithm selection
4. **Memory Management**: Uses raw pointers with explicit delete in destructors

## Testing Strategy

The project uses a **manual testing approach** through the main executable:

1. **Build Testing**: Verify index construction completes without errors
2. **Search Testing**: Compare search results against ground truth
3. **Recall Measurement**: Automatic recall@K calculation
4. **Performance Metrics**: 
   - Search time per query
   - Distance computation count
   - Hop count (graph traversal steps)
   - Memory footprint

### Evaluation Metrics

The code automatically outputs:
```
search time: <seconds_per_query>
DistCount: <number_of_distance_computations>
HopCount: <number_of_hops>
<K> NN accuracy: <recall_value>
```

## Important Implementation Details

### Thread Safety

- OpenMP is used for parallel graph construction
- Mutex locks protect shared data structures (`std::mutex` in node classes)
- Thread-local random number generators

### Distance Computation

- **Embedding distance**: AVX2-optimized squared Euclidean distance normalized by max distance
- **Spatial distance**: 2D Euclidean distance normalized by max distance
- **Combined distance**: `alpha * e_dist + (1 - alpha) * s_dist`

### Graph Structures

- **HNSW**: Multi-layer graph with entry point
- **DEG**: Single-layer graph with:
  - Pareto frontier-based edge selection
  - Alpha range metadata per edge
  - Multiple entry points

### Memory Management

- Large arrays use `mm_malloc` for aligned memory
- Custom `Array<T>` template class for safe array handling
- Graph data stored in contiguous memory where possible

## Security Considerations

1. **File I/O**: Binary file reading without bounds checking - ensure valid input files
2. **Memory**: Large memory allocations for datasets - check available RAM
3. **Path Handling**: Hardcoded absolute paths in main.cpp - modify for your environment
4. **No Input Sanitization**: Command-line arguments are converted directly to numbers

## Common Modifications

### Adding a New Dataset

Edit `include/set_para.h`:
1. Add dataset name check in `set_data_path()`
2. Define file paths for `.fvecs` and `.ivecs` files
3. Add parameter configuration in algorithm-specific functions

### Changing Dataset Root Path

Edit `test/main.cpp` line ~265:
```cpp
std::string dataset_root = R"(/your/path/to/dataset/)";
```

### Adjusting Thread Count

Edit `test/main.cpp` line ~269:
```cpp
parameters.set<unsigned>("n_threads", <num_threads>);
```

## Troubleshooting

### Build Issues

- **OpenMP not found**: Ensure `libgomp` is installed
- **Boost not found**: Set `BOOST_ROOT` in CMakeLists.txt
- **AVX2 errors**: Remove `-march=native` or add `-mavx2` flag

### Runtime Issues

- **File not found**: Check hardcoded paths in main.cpp and set_para.h
- **Segmentation fault**: Verify dataset files match expected dimensions
- **Out of memory**: Reduce batch sizes or use smaller datasets

## References

- Project is based on research paper on Dynamic Edge Navigation Graph
- Implements concepts from HNSW (Malkov & Yashunin), NSG (Fu et al.), and NN-Descent (Dong et al.)
- R-Tree implementation based on Greg Douglas's RTree template
