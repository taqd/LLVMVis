#include "visualize.hpp"

///////////////////////////////////////////////////////////////////
// Graph Configuation - Is the basis of config.json which is our //
// connection to visualizations javascript.			 //
///////////////////////////////////////////////////////////////////

vector<int> get_config(int nodes) {
  //Create different configurations based on number of nodes
  int huge_graph = 1000;
  int large_graph = 80;
  int medium_graph = 20;
  vector<int> config;
    
  if (nodes < medium_graph) {
    int linkDistance = 100; config.push_back(linkDistance);
    int charge = -1000; config.push_back(charge);
    int ticksWithoutCollisions = 100; config.push_back(ticksWithoutCollisions);
    int maxLineChars = 32; config.push_back(maxLineChars);
  } else if (nodes < large_graph) {
    int linkDistance = 20; config.push_back(linkDistance);
    int charge = -400; config.push_back(charge);
    int ticksWithoutCollisions = 100; config.push_back(ticksWithoutCollisions);
    int maxLineChars = 8; config.push_back(maxLineChars);     
  } else if (nodes < huge_graph) {
    int linkDistance = 5; config.push_back(linkDistance);
    int charge = -200; config.push_back(charge);
    int ticksWithoutCollisions = 100; config.push_back(ticksWithoutCollisions);
    int maxLineChars = 4; config.push_back(maxLineChars); 
  } else {
    int linkDistance = 0; config.push_back(linkDistance);
    int charge = -32; config.push_back(charge);
    int ticksWithoutCollisions = 200; config.push_back(ticksWithoutCollisions);
    int maxLineChars = 0; config.push_back(maxLineChars); 
  }
  int numColors = 12; config.push_back(numColors);
  int labelPadL = 2; config.push_back(labelPadL);
  int labelPadR = 2; config.push_back(labelPadR);
  int labelPadT = 2; config.push_back(labelPadT);
  int labelPadB = 2; config.push_back(labelPadB);
  int labelMarginL = 3; config.push_back(labelMarginL);
  int labelMarginR = 3; config.push_back(labelMarginR);
  int labelMarginT = 2; config.push_back(labelMarginT);
  int labelMarginB = 2; config.push_back(labelMarginB);
  
  return config;
}


//////////////////////////////////
// Graph/node creation related. //
//////////////////////////////////

//Wrapper to transform node object into json string
string create_object(node *n) {
  return create_json_object(n->name,
			    n->type,
			    n->group,
			    n->depends);
}

//Standardized format for getting names of things
string get_name(Value *v) {
  string obj_name = v->getName();
  if (obj_name == "") {

    //Name unnamed objects after their instruction opcode if they are
    //an instruction
    if (Instruction *i = dyn_cast<Instruction>(v)) {
      string group = i->getOpcodeName();
      obj_name = group + nameMap[v];
    }

    //Name them after their 'print' function,
    //TODO: Sanitize this
    if (obj_name == "") {
      //If opcode, then try to get its type
      string tmp;
      raw_string_ostream rso(tmp);
      v->getType()->print(rso);
      string type = rso.str();    
      obj_name = type + nameMap[v];
    }

    //If all else fails, use a ID number
    if (obj_name == "") 
      obj_name = nameMap[v];
  }  
  return obj_name;
}

string get_name(BasicBlock *b) {
  string obj_name = b->getName();  
  return obj_name;
}

string get_name(Loop *l) {
  BasicBlock *block_in_loop = l->getBlocks().front();  
  string obj_name = "Loop #" + loopIDs[block_in_loop];
  return obj_name;
}

string get_name(Function *f) {
  string obj_name = f->getName();  
  return obj_name;
}

string get_name(Module *m) {
  string obj_name = m->getName();  
  return obj_name;
}


//Primary grouping method, shown in legend and influences coloring
//Value types use the LLVM type of that value
string get_type(Value *v) {
  string type = "";
  if (SHOW_INSTRUCTION_TYPE) {
    string tmp;
    raw_string_ostream rso(tmp);
    v->getType()->print(rso);
    type = rso.str() + " ";
  }
  if (SHOW_INSTRUCTION_LOOP) {
    type = "Not in a loop";
    if (isa<Instruction>(v)) {
      Instruction *i = dyn_cast<Instruction>(v);
      BasicBlock *parent = i->getParent();
      if (loopIDs[parent].size() > 0)
	type = "Loop #" + loopIDs[parent];
    } 
  }  
  return type;
}

//BasicBlock type is the loop it is in
string get_type(BasicBlock *b) {
  string type = "Not in loop";
  if (loopIDs[b].size() > 0)
    type = "Loop #" + loopIDs[b];
  return type;
}

//Function type is its return type
string get_type(Function *f) {
  //Don't show the types for function declarations
  if (f->isDeclaration())
    return "Function Declaration";  
  string type = "";
  string tmp;
  raw_string_ostream rso(tmp);
  f->getReturnType()->print(rso);
  type = rso.str();
  return type;
}

//Secondary grouping method, shown in legend and influences coloring
//Values can be grouped on the type of instruction (eg. add)
string get_group(Value *v) {
  string group = "";
  if (SHOW_INSTRUCTION_GROUP) {
    if (isa<Instruction>(v)) {
      Instruction *i = dyn_cast<Instruction>(v);
      group = i->getOpcodeName();
    }
  }
  return group;
}

//Basic blocks can be grouped on various properties
string get_group(BasicBlock *b) {
  string group = "";
  // if (numChildren(b) == 0)
  //   group = "Returns";
  // if (numParents(b) == 0)
  //   group = "Entry";
  // if (loopIDs[b].size() == 0)
  //   group = "Not in loop";
  return group;
}

//Functions can be grouped on various properties
string get_group(Function *f) {
  string group = "";
  if (f->isVarArg())
    group = "Variable Arguments";
  if (f->doesNotReturn())
    group = "Does not return";
  return group;
}

//Replace block names with a clickable links which highlights object in graph
string change_blocks_to_links(Function *f, string input) {
  for (BasicBlock &b : *f) {
    replaceAll(input, get_name(&b) + ": ", "{{" + get_name(&b) + "}}" + ": "); 
    replaceAll(input, "%" + get_name(&b) + " ", "%{{" + get_name(&b) + "}} "); 
    replaceAll(input, "%" + get_name(&b) + ",", "%{{" + get_name(&b) + "}},"); 
    replaceAll(input, "%" + get_name(&b) + "\n", "%{{" + get_name(&b) + "}}\n"); 
  }
  return input;
}


//Create a helpful/additional function node that summarizes the whol
//function displayed in this view
node* create_function_node(Function *f, vector<string> folders, bool links) {
  //Set up the basics
  node *n = new node(f);  
  n->name = get_name(f);
  n->type = "Helper";
  n->group = "";
  n->metadata = get_metadata(f,folders); //Borrowed from CF Module view  

  //Change basic block names to clickable links
  if (links)
    n->metadata = change_blocks_to_links(f,n->metadata);

  vector<string> depends;
  n->depends = depends;  
  n->json = create_object(n);

    //Place at top left
  set_Y_position(n,0,1);
  set_X_position(n,1,1);


  return n;
}

//Metadata for a loop
string get_metadata(Loop *l, vector<string> folders, bool update) {
  string header = "<b>Loop data:</b>\n";
  Function *parentFun = NULL; //FIXME, this breaks build_metadata
  
  //Create the syntax highlight code region
  string code = header + syntax_beg;
  for (BasicBlock *b : l->getBlocks()) {
    code += print(b);
    if (parentFun == NULL)
      parentFun = b->getParent();
  }
  code += syntax_end;

  //Transform basicblock names to links to click on
  replaceAll(code,"_","\\_");
  if (update)
    code = change_blocks_to_links(l->getBlocks().front()->getParent(),code);
   
  return build_metadata(parentFun,code,folders);
}

//Create helpful/additional loop nodes. These are placed in the top
//right and corner and are used to access more metadata
node *create_loop_node(Loop *l, vector<string> folders, bool control_flow) {
  //Set up the basics
  node *n = new node((Value*)l);  
  n->name = get_name(l);
  n->type = n->name;
  n->group = "";
  n->metadata = get_metadata(l,folders,control_flow);  

  string name = n->name;

  //Connect the basic blocks this loop with white lines. We don't
  //connect on dataflow views as there can be many nodes and it is
  //slow as it is.
  vector<string> depends;
  if (control_flow) {
    //Link to blocks for control flow views
    for (BasicBlock *b : l->getBlocks()) {
      depends.push_back(get_name(b));

      node *dep = new node();
      dep->name = get_name(b);
      set_link_strength(dep,n,0.0);      
      set_link_color(dep,n,"white");

      for (auto *dep_const : dep->constraints)
	n->constraints.push_back(dep_const);
    }
  } else {
    //Link to instructions for data flow views
    for (BasicBlock *b : l->getBlocks()) {
      for (Instruction &i : *b) {
	if (hide(i)) continue;
	
	depends.push_back(get_name(&i));

	node *dep = new node();
	dep->name = get_name(&i);
	set_link_strength(dep,n,0.0);      
	set_link_color(dep,n,"white");

	for (auto *dep_const : dep->constraints)
	  n->constraints.push_back(dep_const);
      }
    }
  }
  n->depends = depends;
  n->json = create_object(n);

  //Place at the top left
  set_Y_position(n,0.1,1);
  set_X_position(n,0.9,1);

  //When connected, it has no pull on the nodes
  set_node_strength(n,.0001);

  return n;
}

//Create a bunch of handy links to related views
string go_to_related(Function *f) {
  string links = "";
  string ctrl_module_folder = "Module_Control_" + sanitize(get_name(f->getParent()));
  string ctrl_module_link = "[Control Flow](" + root_address + ctrl_module_folder + ")";
  
  string ctrl_function_folder = "Function_Control_" + sanitize(get_name(f));
  string ctrl_function_link = "[Control Flow](" + root_address + ctrl_function_folder + ")";
  
  string df_function_folder = "Function_Data_" + sanitize(get_name(f));
  string df_function_link = "[Data Flow](" + root_address + df_function_folder + ")";

  links += "<b>Jump to Module:</b> (" + sanitize(get_name(f->getParent())) + ") " + ctrl_module_link + "<br />";
  links += "<b>Jump to Function:</b> (" + get_name(f) + ") " + ctrl_function_link + ", " + df_function_link + " <br />";

  return links;
}

//Create a list of other views to other functions
string get_all_views(vector<string> folders) {
  string views = "<b>Other views:</b><br />";
  for (string f : folders) {
    string address = root_address + f;
    string name = f;
    replaceAll(name,"_","\\_");
    string hyperlink = "[" + name + "](" + address + " \"" + f + "\")<br />";
    views += hyperlink;
  }
  return views;
}

//Metadata for a function
string build_metadata(Function *f, string code, vector<string> folders) {
  //Link to enter/open this function
  string links;
  if (f && !f->isDeclaration()) 
    links = go_to_related(f);
  
  //Create a list of other views to other functions
  string views = get_all_views(folders);
  
  //Finally create the big metadata string to be saved in .mkdn
  string metadata = mkdn_header + "\n" + links + "\n<br />" + code + "<br />"  + views;
   
  return metadata;
}

 
////////////////////////////////////////////////////////
// Constraint stuff - eventually saved in config.json //
////////////////////////////////////////////////////////

//Set constraints on where the nodes are placed
void set_Y_position(node *n, float loc, float weight) {
  if (loc < 0 || loc > 1)
    outs() << "Warning location is set beyond graph bounds\n";
  constraint *c = new constraint();
  c->name = n->name;
  c->type = "position";
  c->X = "y";
  c->value = to_string(loc);
  c->Y = "weight";
  c->weight = to_string(weight);
  n->constraints.push_back(c);
}

//Set constraints on where the nodes are placed
void set_X_position(node *n, float loc, float weight) {
  constraint *c = new constraint();
  c->name = n->name;
  c->type = "position";
  c->X = "x";
  c->value = to_string(loc);
  c->Y = "weight";
  c->weight = to_string(weight);
  n->constraints.push_back(c);
}

//Set constraints on all a nodes dependency links
void set_node_strength(node *n, float value) {
  constraint *c = new constraint();
  c->name = n->name;
  c->type = "strength";
  c->X = "strength";
  c->value = to_string(value);
  c->Y = "weight"; //Ignored
  c->weight = "1.0"; //Ignored
  n->constraints.push_back(c);
}

void set_node_width(node *n, float value) {
  constraint *c = new constraint();
  c->name = n->name;
  c->type = "width";
  c->X = "width";
  c->value = to_string(value);
  c->Y = "weight"; //Ignored
  c->weight = "1.0"; //Ignored
  n->constraints.push_back(c);
}

//Set constraint on the link between two nodes
void set_link_strength(node *source, node *target, float value) {
  if (value < 0) {
    outs() << "Link strength cannot be negative\n";
    return;
  }
  constraint *c = new constraint();
  c->name = source->name;
  c->type = "linkStrength";
  c->X = "target";
  c->value = "\"" + target->name + "\"";
  c->Y = "strength"; 
  c->weight = to_string(value); 
  source->constraints.push_back(c);
}

void set_link_width(node *source, node *target, float value) {
  constraint *c = new constraint();
  c->name = source->name;
  c->type = "linkWidth";
  c->X = "target";
  c->value = "\"" + target->name + "\"";
  c->Y = "width"; 
  c->weight = to_string(value); 
  source->constraints.push_back(c);
}

void set_link_color(node *source, node *target, string value) {
  constraint *c = new constraint();
  c->name = source->name;
  c->type = "linkColor";
  c->X = "target";
  c->value = "\"" + target->name + "\"";
  c->Y = "color";
  c->weight = "\"" + value + "\""; 
  source->constraints.push_back(c);
}


/////////////////////////////////////////////
// Visualization specific helper functions //
/////////////////////////////////////////////

//Create formatting such as "400-410,411" from a vector
string format_as_range(vector<int> lines) {
  string lineStr = "";
  sort(lines.begin(),lines.end());

  int last = -42; 
  bool series = false;
  for (int l : lines) {
    if (l == last + 1 && !series) {
      series = true;
      lineStr += to_string(l);
    }
    else if (l == last + 1 && series)
      continue;
    else if (l != last + 1 && series) {
      series = false;
      lineStr += "-" + to_string(l) + ",";
    }
    else 
      lineStr += to_string(l) + ",";
    last = l;
  }
  return lineStr;
}

//Return true for instructions we wish to hide
bool hide(Instruction &i) {
  //Hide unconditional branches
  if (isa<BranchInst>(i)) { 
    BranchInst *tmp = dyn_cast<BranchInst>(&i);
    if (tmp->getNumSuccessors() == 1)
      return true;
  }

  //Hide calls to hidden functions
  if (CallInst *ci = dyn_cast<CallInst>(&i)) { 
    Value *called = ci->getCalledValue()->stripPointerCasts();
    string called_name = dyn_cast<Function>(called)->getName();
    if (hideCallsTo.find(called_name) != string::npos)
      return true;
  }

  //Hide unreachable instructions
  if (isa<UnreachableInst>(i)) 
    return true;

  //Todo: Hide metadata nodes
  
  return false;
}

//The d3 visalization doesn't use curved edges and thus does not
//support self loops, for this reason we create a dummy object that
//points back to the original
void create_self_loop(node *n) {
  //Make the dummy dependant on the original, and original on the
  //dummy
  vector<string> self_loop_dep;  
  self_loop_dep.push_back(n->name);
  n->depends.push_back(n->name+"_SelfLoop");

  //Create the dummy node, and set it as part of this nodes
  //'object'. This makes the original node actually two nodes
  string self_loop = create_json_object(n->name+"_SelfLoop",
					n->type,
					n->group,
					self_loop_dep);
  //  self_loop = self_loop + ",";
  if (n->json == "")
    n->json = self_loop;
  else
    n->json += "," + self_loop;
}

//Return true if the specified value is defined in the extracted
//region.
bool definedInRegion(vector<BasicBlock *> &Blocks, Value *V) {
  if (Instruction *I = dyn_cast<Instruction>(V))
    if (count(Blocks.begin(),Blocks.end(),I->getParent()) > 0)
      return true;
  return false;
}

//Find the "inputs" and "outputs" of the function which will determine
//which nodes are placed at top/bottom. Inputs are defined as a value
//not defined within the body of the function (function arguments and
//global values). Outputs are defined as a value with no child users
//(there is nothing that uses the output)
void findInputOutputs(Function *f, vector<Value*> &Inputs, vector<Value*> &Outputs) {
  vector<BasicBlock*> Blocks;
  for (BasicBlock &b : *f)
    Blocks.push_back(&b);
  
  for (BasicBlock &b : *f) {
    for (Instruction &i : b) {
      if (CallInst *ci = dyn_cast<CallInst>(&i)) {
	Value *called = ci->getCalledValue()->stripPointerCasts();
	string called_name = dyn_cast<Function>(called)->getName();
	if (hideCallsTo.find(called_name) != string::npos) continue;
      }
      for (Use &op : i.operands()) {
	if (isa<Function>(op)) continue;
	if (isa<BasicBlock>(op)) continue;
	//	if (!SHOW_GLOBAL_VALUES && isa<Constant>(op)) continue; //Do not show constants as inputs if set
	if (isa<Constant>(op) && !isa<GlobalValue>(op)) continue; 
	if (!definedInRegion(Blocks, op))
	  Inputs.push_back(op);
      }

      if (i.user_empty()) {
	if (isa<BranchInst>(i)) continue;
	if (isa<UnreachableInst>(i)) continue;
	Outputs.push_back(&i);
      }
    }    
  }
}


////////////////////////////////////
// Generic LLVM Helper Functions  //
////////////////////////////////////

//Make nice strings
string sanitize(string name)
{
  name.erase(remove(name.begin(), name.end(),'"'), name.end());
  name.erase(remove(name.begin(), name.end(),'.'), name.end());
  name.erase(remove(name.begin(), name.end(),'<'), name.end());
  name.erase(remove(name.begin(), name.end(),'>'), name.end());
  //  name = name.substr(0,100);
  return name;
}

//Replaces all occurences of "from", in string "str", with "to" 
void replaceAll(std::string& str, const std::string& from, const std::string& to) {
    if(from.empty())
        return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

//Returns the print() of an object. This is used in metadata creation
string print(Value *i)
{
  string tmp;
  
  raw_string_ostream rso(tmp);
  i->print(rso);
  string rso_str = rso.str();

  return rso_str;
}

string print(Metadata *m)
{
  string tmp;
  
  raw_string_ostream rso(tmp);
  m->print(rso,nullptr,true);
  string rso_str = rso.str();

  return rso_str;
}

//Returns the pointer address for a value. Used to provide a unique ID
//for any value
string get_val_addr(Value *i)
{
  string tmp;
  raw_string_ostream rso(tmp);
  rso << i;
  return rso.str();
}

//Find how many children this block has
int numChildren(BasicBlock *b)
{
  if (!b) return 0; 
  if (!b->getTerminator()) return 0;
  return b->getTerminator()->getNumSuccessors();
}

//Find how many predecessors this basic block has
int numParents(BasicBlock *b)
{
  int count = 0;
  if (!b) return 0;
  if (pred_begin(b) == pred_end(b)) return 0;
  for (pred_iterator PI = pred_begin(b), E = pred_end(b);
       PI != E; ++PI) {
    count++;
  }
  return count;
}

//Find all the functions that call 'source'
vector<Function*> find_callers(Function *source)
{
  vector<Function*> WL;
  for (Function &f : *source->getParent()) {
    for (BasicBlock &b : f) {
      for (Instruction &i : b) {
	//Only look at callsites
	CallSite cs(&i);
	if (!cs.getInstruction()) continue;
	Value *called = cs.getCalledValue()->stripPointerCasts();

	//Check that this function calls the source, and that we
	//haven't added it to the list already
	if (dyn_cast<Function>(called) == source &&
	    find(WL.begin(),WL.end(),&f) == WL.end())
	  WL.push_back(&f);
      }
    }
  }
  return WL;
}

//Find all the functions that 'source' calls
vector<Function*> find_called(Function *source)
{
  vector<Function*> WL;
  for (BasicBlock &b : *source) {
    for (Instruction &i : b) {
      //Only look at callsites
      CallSite cs(&i);
      if (!cs.getInstruction()) continue;
      Function *called = cs.getCalledFunction();
      if (!SHOW_FUNCTION_DECLARATIONS && called->isDeclaration()) continue;
      if (find(WL.begin(),WL.end(),source) == WL.end())
	WL.push_back(called);
    }
  } 
  return WL;
}

//Count the number of times source calls target
int count_calls(Function *source, Function *target)
{
  int calls = 0;
  for (BasicBlock &b : *source) {
    for (Instruction &i : b) {
      //Only look at callsites
      CallSite cs(&i);
      if (!cs.getInstruction()) continue;
      Function *called = cs.getCalledFunction();
      if (called == target)
	calls++;
    }
  }
  
  return calls;
} 

//////////////////////////////////////
// JSON file writing stuff	    //
// TODO: use a proper json library  //
//////////////////////////////////////

//Create a string of objects that constains all the nodes which will
//be written to file. 
string create_json_object(string name, string type, string group, vector<string> depends) {

  //Create the object string
  string name_str_A = "\t\t\"name\" : \"";
  string name_str_B = "\",";
  string type_str_A = "\t\t\"type\" : \"";
  string type_str_B = "\",";  
  string group_str_A = "\t\t\"group\" : \"";
  string group_str_B = "\",";
  string depends_str_A = "\t\t\"depends\" : [";
  string depends_str_B = "\t\t]";
  
  string object = "{\n" +
    type_str_A + type + type_str_B + "\n" +
    name_str_A + name + name_str_B + "\n" +
    group_str_A + group + group_str_B + "\n" +
    depends_str_A +  "\n";

  //Add all dependencies, and ensure the last comma is handled correctly
  bool first = true;
  for (string s : depends) {
    if (!first)
      object += ",\n";
    object += "\t\t\t\"" + s + "\"";
    first = false;
  }
  object += "\n";
  object += depends_str_B + "\n";



  object += "\t}";
  
  return object;
}

//Create objects.json which stores all objects
void create_objects_file(string folder, vector<node*> nodes ) {
  fstream File;
  File.open (folder + "objects.json", fstream::out);
  File << "[\n";
  bool first = true;
  for (node *n : nodes) {
    if (n->json.size() == 0) continue;
    if (first) {
      File << "\t";
      first = false;
    } else {
      File << ", ";
    }
    File << n->json;
  }
  File << "\n]\n";
  File.close();
}

//Create the node types which will be saved in the config file
string create_config_types(vector<node*> nodes) {
  string type_str = "";
  for (uint i=0;i<nodes.size();i++) {
    string type_short = nodes[i]->type;
    //    if (type_short.size() == 0) continue;
    string type_long = "";//nodes[i]->type_long;
    if (i>0) type_str += ",\n";
    type_str += "\t\t\"" + type_short + "\" : {\n"
      + "\t\t\t\"short\" : \"" + type_short + "\",\n"
      + "\t\t\t\"long\"  : \"" + type_long  + "\" \n"
      + "\t\t}";
  }
  return type_str;
}

//Create the node constraints which will be saved in the config file
string create_config_constraints(vector<node*> nodes) {
  string constraint_str = "";
  
  for (uint i=0;i<nodes.size();i++) {
    for (uint j=0;j<nodes[i]->constraints.size();j++) {
      constraint *c = nodes[i]->constraints[j];
      
      if (constraint_str != "") constraint_str += ", ";
      else constraint_str += "\t\t";
      
      constraint_str = constraint_str + "{\n"
	+ "\t\t\t\"has\" : { \"name\" : \"" + c->name + "\" },\n"
	+ "\t\t\t\"type\" : \"" + c->type + "\",\n"
	+ "\t\t\t\"" + c->X + "\" : " + c->value + ",\n"
	+ "\t\t\t\"" + c->Y + "\" : " + c->weight + "\n"
	+ "\t\t}";
    }
  }
  return constraint_str;
}

//Create the configuration file that determines the size of the
//graphing area, the forces used to layout the nodes and padding
//sizes. This is written into config.json for every view
void create_config_file(vector<int> config,
			string folder,
			string title,
			vector<node*> nodes) {
    
  fstream File;
  File.open (folder + "config.json", fstream::out);

  //Start the jsonness
  File << "{\n";

  string title_str_A = "\t\"title\" : \"";
  string title_str_B = "\",";
  File << title_str_A << title << title_str_B << "\n";

  string graph_str_A = "\t\"graph\" : {";
  string graph_str_B = "\t},";
  File << graph_str_A << "\n"
       << "\t\t\"linkDistance\" : " << config[0] << ",\n"
       << "\t\t\"charge\" : " << config[1] << ",\n"    
       << "\t\t\"ticksWithoutCollisions\" : " << config[2] << ",\n"
       << "\t\t\"maxLineChars\" : " << config[3] << ",\n"
       << "\t\t\"numColors\" : " << config[4] << ",\n"    
       << "\t\t\"labelPadding\" : {\n"
       << "\t\t\t\"left\" : " << config[5] << ",\n"
       << "\t\t\t\"right\" : " << config[6] << ",\n"
       << "\t\t\t\"top\" : " << config[7] << ",\n"
       << "\t\t\t\"bottom\" : " << config[8] << "\n"
       << "\t\t},\n"
       << "\t\t\"labelMargin\" : {\n"
       << "\t\t\t\"left\" : " << config[9] << ",\n"
       << "\t\t\t\"right\" : " << config[10] << ",\n"
       << "\t\t\t\"top\" : " << config[11] << ",\n"
       << "\t\t\t\"bottom\" : " << config[12] << "\n"
       << "\t\t}\n"
       << graph_str_B << "\n";

  string types_str_A = "\t\"types\" : {";
  string types_str_B = "\t},";
  File << types_str_A << "\n"
       << create_config_types(nodes)
       << "\n" << types_str_B << "\n";

  string constraints_str_A = "\t\"constraints\" : [";
  string constraints_str_B = "\t]";
  
  File << constraints_str_A << "\n"
       << create_config_constraints(nodes)
       << constraints_str_B << "\n";
  
  File << "\n}\n";
  File.close();
}

//  Create the metadata files, one for each node. Saved into the
//  folder of that view with the filename of <object_name>.mkdn. This
//  file supports markdown formatting and javascript
void create_data_files(string folder, vector<node*> nodes) {
  fstream File;
  for (node *n : nodes) {
    string obj_name = n->name;
    string filename = folder + obj_name + ".mkdn";
    File.open (filename, fstream::out);
    File << n->metadata;
    File.close();
  }
}
