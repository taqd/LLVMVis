#include "llvm/Transforms/Utils/Local.h" //outs()
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/DebugInfo.h"
#include <fstream>
#include <dirent.h> //For DIR
#include <iomanip>
#include <string>
#include <unordered_map>
#include <sys/stat.h>
#include <unistd.h>


using namespace std;
using namespace llvm;

/* General view settings */
#define TRUNCATE_CODE_MKDN true
#define TRUNCATE_AT 1000

/* View settings for MODULE CONTROL FLOW view */
#define CREATE_CF_MODULE_VIEW true
#define SHOW_FUNCTION_DECLARATIONS false

/* View settings for FUNCTION CONTROL FLOW views */
#define CREATE_CF_FUNCTION_VIEWS true

/* View settings for FUNCTION DATA FLOW views */
#define CREATE_DF_FUNCTION_VIEWS true
#define CONNECT_GLOBAL_VALUES true
#define SHOW_INSTRUCTION_TYPE false //eg. i32
#define SHOW_INSTRUCTION_GROUP false //eg. store
#define SHOW_INSTRUCTION_LOOP true  //eg. Loop #1 (overloaded on type)
#define GROUP_DF_BY_CF false

//The root address used in hyperlinks
static string root_address = "http://137.82.252.51/graph.php?dataset=";
static string dataFolder = "data/";
static string epochFile = "data/count.txt";

//List of functions names that will be hidden from all views
static string hideCallsTo = "puts,printf,llvm.dbg.value";

//Header in the markdown file to provide syntax highlighting
static string prettify_theme = "default"; //Options found in code-prettify/styles/
static string mkdn_header = "<script src=\"code-prettify/loader/run_prettify.js?autoload=true&skin="+prettify_theme+"\" ></script> \n <script src=\"code-prettify/src/lang-llvm.js\"></script>\n";
static string syntax_beg = "<pre class=\"prettyprint lang-llvm \">";
static string syntax_end = "</pre>\n";

//Names for loops and he unnamed
extern unordered_map<BasicBlock*,string> loopIDs;
extern unordered_map<Value*,string> nameMap;

//Container for node position, and various link settings such as edge
//width, and color. Constraints are optional, but without them nodes
//positioning and layout is governed entirely by the force algorithm.
struct constraint {
  string name, type, X, Y, value, weight;
};

//Container to hold the nodes and various properties about them. Todo:
//more documentation here.
struct node {
  string name, type, group, metadata, json;
  vector<string> depends;
  vector<constraint*> constraints;
  Value *original;
  node() { original = NULL; }
  node(Value *val) { original = val; }
};


/* 
   Graph building and configuration - in create_object.cpp
*/
//Create the configuation settings for the graph
vector<int> get_config(int nodes);

//Create a node in the graph to be written into objects.json
string create_object(node *n);

//Standardized format for getting names of things
string get_name(Value *v);
string get_name(BasicBlock *b);
string get_name(Loop *l);
string get_name(Function *f);
string get_name(Module *m);

//Primary grouping method, shown in legend and influences coloring
string get_type(BasicBlock *b);
string get_type(Function *f);
string get_type(Value *v);

//Secondary grouping method, shown in legend and influences coloring
string get_group(BasicBlock *b);
string get_group(Function *f);
string get_group(Value *v);

//Create the metadata shown on the right side bar, code is user defined.
string get_metadata(Function *f, vector<string> folders);
string get_metadata(BasicBlock*b, vector<string> folders);
string build_metadata(Function *f, string code, vector<string> folders);

//Set constraints on where the nodes are placed
void set_Y_position(node *n, float loc, float weight);
void set_X_position(node *n, float loc, float weight);

//Set constraints on all a nodes dependency links
void set_node_strength(node *n, float value);
void set_node_width(node *n, float value);

//Set constraint on the link between two nodes
void set_link_strength(node *source, node *target, float value);
void set_link_width(node *source, node *target, float value);
void set_link_color(node *source, node *target, string value);


/*
  Helpful/additional function/loop nodes. These are placed in the top
  right and corner and are used to access more metadata
*/
node* create_function_node(Function *f, vector<string> folders, bool links);
node *create_loop_node(Loop *l, vector<string> folders, bool control_flow);


//Find the "inputs" and "outputs" of the function. (Modified from CodeExtractor)
void findInputOutputs(Function *f, vector<Value*> &Inputs, vector<Value*> &Outputs);


/*
  Visualization helper functions
*/
//Create formatting such as "400-410,411" from a vector
string format_as_range(vector<int> lines);

//Create a dummy node to allow for self loops
void create_self_loop(node *n);

//Allow block names to be clicked
string change_blocks_to_links(Function *f, string input);

//Return true for instructions we wish to hide
bool hide(Instruction &i);


/* 
   LLVM Helper Functions 
*/
//Replaces all occurences of "from", in string "str", with "to" 
void replaceAll(std::string& str, const std::string& from, const std::string& to);

//Returns the print() of an object. This is used in metadata creation
string print(Value *i);
string print(Metadata *m);

//Returns the pointer address for a value. Used to provide a unique ID
//for any value
string get_val_addr(Value *i);

//Find how many children this block has
int numChildren(BasicBlock *b);
//Find how many predecessors this basic block has
int numParents(BasicBlock *b);

//Make strings nice
string sanitize(string name);

//Find all the functions that call this function
vector<Function*> find_callers(Function *source);

//Find all the function that this function calls
vector<Function*> find_called(Function *source);

//Count the number of times source calls target
int count_calls(Function *source, Function *target);



/* 
   JSON file writing stuff
*/
//Create a string of objects that constains all the nodes which will
//be written to file
string create_json_object(string name, string type, string group, vector<string> depends);

//Create objects.json which stores all objects
void create_objects_file(string folder, vector<node*> nodes);

//Create the node types which will be saved in the config file
string create_config_types(vector<node*> nodes);

//Create the node constraints which will be saved in the config file
string create_config_constraints(vector<node*> nodes);

//Create the configuration file that determines the size of the
//graphing area, the forces used to layout the nodes and padding
//sizes. This is written into config.json for every view
void create_config_file(vector<int> config,
			string folder,
			string title,
			vector<node*> nodes);

//  Create the metadata files, one for each node. Saved into the
//  folder of that view with the filename of <object_name>.mkdn. This
//  file supports markdown formatting and javascript
void create_data_files(string folder, vector<node*> nodes);
