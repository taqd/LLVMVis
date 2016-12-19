# LLVM Visualize

LLVM Visualize (LLVMVis) is an interactive program
visualization/exploration/debugging tool for LLVM. It uses a compiler
pass as a backend and transforms several of LLVM's internal data
structures into json format, to then be visualized in your webbrowser
using the D3 javascript library.

## Features
* Interactive acyclic graph with automatic force layout.
* Additional, and totally customizable, 'additional information' for every node.
* Ability to manually set node positions, and edge widths/colours.
* Prebuilt control flow, data flow, and call graph views
* An easy to use API with several well documented examples.
* A history across successive visualizations allowing for changes in code to be easily seen and understood.
* Extended information on every node, such as the IR of the object, along with its debug information. This is easily adapted to show anything else that the developer desires.
* Ability to navigate through a program, from function to function, or block to block in a single, easy to use interface.
* Themeable!

## Installation
* Add the source code to your LLVM source directory
```bash
git clone https://github.com/tqdwyer/LLVMVis.git ~/<LLVM SRC>/lib/Analysis
add “add_subdirectory(visualize)” to <LLVM SRC>/lib/Analysis/CMakeLists.txt
#Re-build your llvm
```

* Install a webserver with php
```bash
sudo apt-get install apache2 php libapache2-mod-php
```

* Set your configuration settings in visualize.hpp

* Run the visualization pass
```bash
opt –load visualize.so –visualize -o dump < your_input.bc
```

* Now check out your webserver! The pass will automatically sync the data files to /var/www/http/data

## Demonstration

Try it out yourself at [http://137.82.252.51/graph.php?dataset=Module_Control_stdin](http://137.82.252.51/graph.php?dataset=Module_Control_stdin).
*NOTE: To access this server you will need to be on the UBC ECE network, I suggest tunneling your browser traffic through ssh.ece.ubc.ca*

### Module Level Control Flow View | [View as album](http://imgur.com/a/034g)

![Control Flow view of Module](https://github.com/tqdwyer/LLVMVis/blob/master/www/html/img/LLVMVis_470lbm_ModuleCF.gif) 

### Function Level Control Flow View | [View as album](http://imgur.com/a/qyvkd)

![Control Flow view of Function](https://github.com/tqdwyer/LLVMVis/blob/master/www/html/img/LLVMVis_470lbm_FunctionCF.gif) 

### Function Level Dataflow view | [View as album](http://imgur.com/a/l8Joj)

![Data Flow view of Function](https://github.com/tqdwyer/LLVMVis/blob/master/www/html/img/LLVMVis_470lbm_FunctionDF.gif) 

### The history feature [View as album](http://imgur.com/a/PCJl0)
![History Feature](https://github.com/tqdwyer/LLVMVis/blob/master/www/html/img/LLVMVis_470lbm_History.gif) 

### Some customization options | [View as album](http://imgur.com/a/cuumf)

![Customization option](https://github.com/tqdwyer/LLVMVis/blob/master/www/html/img/LLVMVis_470lbm_Features.gif) 


## Other details

The code uses a
[d3.js force layout](https://github.com/mbostock/d3/wiki/Force-Layout) to
compute object positions, with
[collision detection](http://bl.ocks.org/mbostock/3231298) to prevent nodes
from overlapping each other, and is based upon the [d3-process-map](https://github.com/nylen/d3-process-map) example.

Nodes are colored by the
[ColorBrewer Set3 scheme](http://colorbrewer2.org/?type=qualitative&scheme=Set3&n=12),
with colors assigned by the combination of the object's `type` and `group`.

To ensure that the arrows on the ends of the links remain visible, the links
only extend to the outer edge of the target object's node.

[SeedRandom](http://davidbau.com/archives/2010/01/30/random_seeds_coded_hints_and_quintillions.html)
has been used to get deterministic random values, and keep graphs
refreshing the same way across epochs.

[CodePrettify](https://github.com/google/code-prettify) has also been
used for syntax highlighting in the markdown files.
