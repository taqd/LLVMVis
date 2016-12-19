#include "visualize.hpp"

//Todo:
// - Create read me
// - Formalize 'theme' ability

//LLVM Stuff
static const char h_name[] = "printGraph Module Pass";
struct visualize : public ModulePass {
  static char ID;
  visualize() : ModulePass(ID) {}
  virtual ~visualize() { }
  virtual void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addPreserved<LoopInfoWrapperPass>();
    AU.setPreservesAll();    
  }
  virtual bool runOnModule(Module &M) override;

};
char visualize::ID = 0;
static RegisterPass<visualize> X("visualize", "Pass to print a nice d3 web visualization graph");


unordered_map<BasicBlock*,string> loopIDs;
unordered_map<BasicBlock*,int> loopNum;
unordered_map<Value*,string> nameMap;
int current_epoch;



/*************************************************************************
  Create a control flow view of the module. This view has nodes
  representing functions, and edges between nodes representing the
  calls between functions. Edge width is dependant on number of calls
  made, and edge colour has been set as an example.
**************************************************************************/

//Create constraints  on the nodes, and  edges. This is where  we have
//manual control over the graph.
void set_constraints(Function *fun_source, node *n) {
  int max_width = 5;
  //Put any function named 'main' at the top (Zero in Y axis, with
  //weight '5' so it isn't pulled down by other nodes too easily
  if (get_name(fun_source)  == "main")
    set_Y_position(n,0,5); 

  //Increase edge width on the number of times the function is called
  for (Function *fun_target : find_called(fun_source))  {   

    //Get the number of times the target is called in the source
    int num_calls = count_calls(fun_source, fun_target);

    //Create a temporary node for a target, must have correct name for
    //target -- but that is all. 
    node *node_target = new node();
    node_target->name = get_name(fun_target);

    //Set the edge width
    set_link_width(n,node_target,min(max_width,num_calls));

    //Set the edge colour of functions called by main.
    if (get_name(fun_source).find("main") != string::npos) {
      set_link_color(n,node_target,"blue");
    }
  }
}

vector<string> get_dependencies(Function *f, node *n) {
  vector<string> depends;
  vector<Function*> callers = find_callers(f);

  for (Function *caller : callers) {
    string dep_name = get_name(caller);
    if (caller == f) {
      create_self_loop(n);
    } else
      depends.push_back(dep_name);
  }
  return depends;
}

//Text to go into the 'IR' tab
string get_ir(Function *f){
  if (!ENABLE_IR) return "Disabled";

  //Get the contents of this function
  string IR = print(f);

  //Build the full metadata page with navigation to related functions,
  //and a list of all available views
  return prep_metadata(IR);
}

//Text for the debug tab
string get_debug(Function *f) {
  if (!ENABLE_DEBUG) return "Disabled";
  
  string debug_header = "Filename: " + get_file(f) + "\nDirectory: " + get_dir(f);
  string debug_content ="";

  //Enable this for line-by-line debug information for the entire
  //function. Warning: SLOW
  // for (BasicBlock &b : *f) {
  //   debug_content += get_blk_metadata(&b);
  // }

  return debug_header + "\n" + debug_content;
}


//Create the full view. This assumes that the folders have already
//been created, and their names are in the 'folders' vector.
void create_control_flow_view(Module &m, vector<string> folders)
{
  vector<node*> nodes;   
  string title = get_name(&m);
  if (VERBOSE)
    outs() << "Creating control flow view for module: " << title;

 
  //Create the objects. Each function is a node.
  for (Function &f : m) {
    if (!SHOW_FUNCTION_DECLARATIONS && f.isDeclaration()) continue;

    //Create a node for this function with default settings
    node *n = new node(&f);
    n->name = get_name(&f); 
    n->type = get_type(&f);
    n->group = get_group(&f);
    n->depends = get_dependencies(&f,n);        
    n->metadata = get_ir(&f);
    n->src = get_debug(&f);
    n->json = create_object(n);
    nodes.push_back(n);    

    //Set the constraints for this node
    set_constraints(&f,n);

    if (VERBOSE) outs() << ".";
  }
  
  //Create objects.json 
  string folder = dataFolder + "Module_Control_" + title + "/";
  create_objects_file(folder,nodes);
  
  //Create config.json
  vector<int> config = get_config(nodes.size());
  create_config_file(config,folder,title,nodes);
  
  //Create the *.mkdn files for each object
  create_data_files(folder,nodes);

  //Print some nice output
  if (VERBOSE)
    outs() << "\t Total Nodes: " << nodes.size() << "\n";

  //Sync the data folder to the web folder
  if (DO_SYNC) system(syncCommand.c_str());        
}



/*************************************************************************
  Create a control flow view of all functions. This view has nodes
  representing basic blocks, and edges between nodes representing the
  branches between blocks. The entry block is put at the top, and all
  blocks which return or exit are placed at the bottom. Blocks are
  colored by loop, and additional loop 'helper' nodes have been added
  to the top LH corner. Another helper node is added to document the
  entire function.
**************************************************************************/

void set_constraints(BasicBlock *b, node *n) {
  //Blocks without parents (entry node) are placed at the top
  if (numParents(b) == 0) {
    set_Y_position(n,0,5);
  }

  //Blocks without children (return or exit) are placed at the bottom
  if (numChildren(b) == 0){
    set_Y_position(n,1,5);
  }  
}

vector<string> get_dependencies(BasicBlock *b, node *n) {
  vector<string> depends;

  //Predecessors of a block == who this block depends on. Thanks llvm.
  for (pred_iterator PI = pred_begin(b), PE = pred_end(b); PI != PE; ++PI) {
    string dep_name = get_name(*PI);
    if ((*PI) == b)
      create_self_loop(n);
    else
      depends.push_back(dep_name);
  }

  return depends;
}

//Text for the IR tab
string get_ir(BasicBlock*b) {
  if (!ENABLE_IR) return "Disabled";  

  //First, get the file/line/col debug information for this block
  string debug = "";
  vector<int> lines, cols;
  for (Instruction &i : *b) {
    //    if (MDNode *N = i.getMetadata("dbg")) {  
    if (i.getMetadata("dbg")) {  
      DebugLoc loc = i.getDebugLoc();
      int line = loc.getLine();
      int col = loc.getCol();
      if (find(lines.begin(),lines.end(),line) == lines.end()) 
	lines.push_back(line);
      if (find(cols.begin(),cols.end(),col) == cols.end()) 
	cols.push_back(col);
    }
  }

  //Format it nicely into a range style
  debug = "File: " + get_file(b->getParent()) + ", Lines: " + format_as_range(lines) + " " + "Cols: " + format_as_range(cols);

  //Get the contents of this block
  string code = debug + print(b);
  
  //Build the full metadata page with navigation to related functions,
  //and a list of all available views
  return prep_metadata(code);
}

//Text for the debug tab
string get_debug(BasicBlock *b) {
  if (!ENABLE_DEBUG) return "Disabled";  
  string debug_header = "Filename: " + get_file(b->getParent()) + "\nDirectory: " + get_dir(b->getParent());
  string debug_content = get_blk_metadata(b);

  return debug_header + "\n\n" + debug_content;
}

void create_control_flow_view(Function &f, vector<string> folders, LoopInfo *LI) {
  vector<node*> nodes;
  string title = get_name(&f);
  if (VERBOSE)
    outs() << "Creating control flow view for function: " << title;

  //Create a independant function node (a helper)
  node *n = create_function_node(&f,folders,true);
  nodes.push_back(n);

  //Create independant loop nodes, without duplicates
  vector<Loop*> WL;
  for (BasicBlock &b : f) {

    //Get the loop this block is in, if any
    Loop *loop = LI->getLoopFor(&b);
    if (!loop) continue;

    //Don't add duplicates
    if (find(WL.begin(),WL.end(),loop) != WL.end()) continue;
    WL.push_back(loop);

    //Create the loop node
    node *n = create_loop_node(loop,folders,true);
    nodes.push_back(n);
  }
  
  //Create the basicblock nodes
  for (BasicBlock &b : f) {
    node *n = new node(&b);
    n->name = get_name(&b);
    n->type = get_type(&b);
    n->group = get_group(&b);
    n->depends = get_dependencies(&b,n); 
    n->metadata = get_ir(&b);
    n->src = get_debug(&b);
    n->json = create_object(n);
    nodes.push_back(n);
    
    set_constraints(&b,n);

    if (VERBOSE) outs() << ".";
  }
    
  //Create objects.json
  string folder = dataFolder + "Function_Control_" + title + "/";
  create_objects_file(folder,nodes);

  //Create config.json
  vector<int> config = get_config(nodes.size());
  create_config_file(config,folder,title,nodes);

  //Create the *.mkdn files for each object
  create_data_files(folder,nodes);

  //Print some nice output
  // int paddingLength = 15;
  // if (paddingLength - title.size() > 0)
  //   title.insert(title.end(),paddingLength - title.size(), ' ');
  if (VERBOSE)
    outs() << "\t Total Nodes: " << nodes.size() << "\n";

  //Sync the data folder to the web folder
  if (DO_SYNC) system(syncCommand.c_str());          
}


/*************************************************************************
  Create a data flow view of a function. This view has nodes
  representing LLVM IR instructions, the functions arguments, and any
  global values used in the function. Data values which are defined
  outside of the function are at the top and considered inputs, values
  which have no successors (has no users) are put at the bottom, and
  considered outputs. Nodes are colored by the loop they are in, or
  their type (eg. i32), or their instruction type (eg. add). Helper
  nodes for the loops, and function have also been added.
**************************************************************************/

static float step = 0;
void set_constraints(Value *v, node *n, vector<Value*> &Inputs, vector<Value*> &Outputs) {  
  if (find(Inputs.begin(),Inputs.end(),v) != Inputs.end()) {
    set_Y_position(n,0,5); //Put input nodes at the top
    set_X_position(n,step,1); //Spread them out over the X dimension
    step = step + 1.0/Inputs.size();
  }

  if (find(Outputs.begin(),Outputs.end(),v) != Outputs.end())
    set_Y_position(n,1,5); //Put the output nodes a the bottom
}

//Text for the IR tab
string get_ir(Value *v) {
  if (!ENABLE_IR) return "Disabled";
  //Get the file/line/col debug information if this is an instruciton
  string debug = "";
  if (Instruction *i = dyn_cast<Instruction>(v)) {
    if (i->getMetadata("dbg")) {
      DebugLoc loc = i->getDebugLoc();
      string line = to_string(loc.getLine());
      string col = to_string(loc.getCol());      
      debug = ";; Line: " + line + ", Column: " + col + "";
    }
  }

  //Get the contents of this value
  string code = debug + "\n" + print(v);
  
  //Add the instruction operands, and parent block in different code blocks
  string other = "\n" + syntax_end + "\n"; //End the last code block
					   //(added automatically in
					   //prep_metadata
  if (Instruction *i = dyn_cast<Instruction>(v)) {    
    for (Use &op : i->operands()) {
      int opNum = op.getOperandNo();
      string opPrefix = "Operand # " + to_string(opNum) + " (" + get_val_addr(op) + ") <br />";
      string opData = print(op);
      other += opPrefix + syntax_beg + opData + syntax_end;
    }
    //code = change_instructions_to_links(parentFun,code);    

    string parentBlk_data = print(i->getParent());
    other += "Parent Block : <br />" + syntax_beg + parentBlk_data + syntax_end;
  } 

  return prep_metadata(code+other);
}

//Create an instruction node. These depend on which other instructions
//or values this instruction uses.
vector<string> get_dependencies(Instruction *i, vector<Value*> &Inputs) {  
  vector<string> depends;

  //An instruction uses its operands, and thus depends on them
  for (Use &u : i->operands()) {

    //If the operand is another instruction, we depend on that instruction
    if (Instruction* op  = dyn_cast<Instruction>(u)) {
      if (!hide(*op)) {
	depends.push_back(get_name(u));
      }
    }
    
    //Note: global value are considered 'Inputs' as they are defined
    //outside of the function, and will automatically be depended on
    //in the next else if, however, as constants such as '42' are
    //considered global values, they can create a lot of unnecessary
    //nodes. Here, we skip those constants.
    else if (!CONNECT_GLOBAL_VALUES && isa<Constant>(u))
      continue;
    
    //If we depend on an input (a function argument) add it.
    else if (find(Inputs.begin(),Inputs.end(),u) != Inputs.end()) 
      depends.push_back(get_name(u));
  }
  return depends;
}
  
void create_data_flow_view(Function &f, vector<string> folders, LoopInfo *LI) {
    vector<node*> nodes;
    string title = get_name(&f);
    if (VERBOSE)
      outs() << "Creating dataflow view for function: " << title;
    
    //Create an function helper node
    node *n = create_function_node(&f,folders,true);    
    nodes.push_back(n);

    //Create loop helper nodes
    if (SHOW_INSTRUCTION_LOOP) {
      vector<Loop*> WL;
      for (BasicBlock &b : f) {
	Loop *loop = LI->getLoopFor(&b);
	if (!loop) continue;
	if (find(WL.begin(),WL.end(),loop) != WL.end()) continue;
	WL.push_back(loop);

	node *n = create_loop_node(loop,folders,false);
	nodes.push_back(n);
      }
    }
      
    //Create the data value input nodes (ones with data defined
    //outside the function)
    vector<Value*> Inputs, Outputs;
    vector<string> empty_set;
    findInputOutputs(&f, Inputs, Outputs);
    for (Value * v : Inputs) {
      node *n = new node(v);
      n->name = get_name(v);
      n->type = get_type(v);
      n->group = get_group(v);
      n->depends = empty_set;
      n->metadata = get_ir(v);
      n->json = create_object(n);
      nodes.push_back(n);
      
      set_constraints(v,n,Inputs,Outputs);

      if (VERBOSE) outs() << ".";
    }
    step = 0; //Local global used in set_constraints

    //Create the data value output nodes (ones with no children)
    for (Value * v : Outputs) {
      node *n = new node(v);
      n->name = get_name(v);
      n->type = get_type(v);
      n->group = get_group(v);
      if (Instruction *i = dyn_cast<Instruction>(v)) {
	if (hide(*i)) continue;
	n->depends = get_dependencies(i,Inputs);
      } else {
	n->depends = empty_set;
      }
      n->metadata = get_ir(v);
      n->json = create_object(n);
      nodes.push_back(n);
      
      set_constraints(v,n,Inputs,Outputs);
      
      if (VERBOSE) outs() << ".";
    }
    step = 0; //Local global used in set_constraints    

    //Add all nodes now (nearly everything in the function, whether
    //it is connected or not).
    for (BasicBlock &b : f) {
      for (Instruction &i : b) {

	//Don't create nodes for some instructions we don't care
	//about, such as unconditional branches, unreachable
	//instructions, metadata nodes, or calls to common functions
	//such as 'printf'.
	if (hide(i)) continue;

	//Create our node
	node *n = new node(&i);
	n->name = get_name(&i);
	n->type = get_type(&i);
	n->group = get_group(&i);
	n->depends = get_dependencies(&i,Inputs);
	n->metadata = get_ir(&i);
	n->json = create_object(n);      		
	nodes.push_back(n);

	if (VERBOSE) outs() << ".";
      }
    }

    //Create objects.json
    string folder = dataFolder + "Function_Data_" + title + "/";
    create_objects_file(folder,nodes);

    //Create config.json
    vector<int> config = get_config(nodes.size());
    create_config_file(config,folder,title,nodes);

    //Create the *.mkdn files for each object
    create_data_files(folder,nodes);

    //Print some nice output
    if (VERBOSE) {
      outs() << "\t Inputs: " << Inputs.size()
	     << "\t Outputs: " << Outputs.size()
	     << "\t Total Nodes: " << nodes.size() << "\n";
    }

    //Sync the data folder to the web folder
    if (DO_SYNC) system(syncCommand.c_str());
}

//Finds the current epoch
string get_epoch(string filename) {
  string currentEpoch = "0";

  fstream File;
  File.open (filename, std::fstream::in);

  //Get previous epoch number, and update to current
  if (File.is_open()) {
    File >> current_epoch;
    currentEpoch = to_string(current_epoch + 1);
    current_epoch++;
    File.close();
  }
  
  //Write current epoch number back to file
  File.open (filename, std::fstream::out | std::fstream::trunc);  
  if (File.is_open()) {
    File << setfill('0') << setw(5)<< currentEpoch; //Pad with zeros
    File.close();
  }

  return currentEpoch;
}

bool visualize::runOnModule(Module &m)
{

  string epochStr = get_epoch(epochFile);
  dataFolder = "data/epoch" + epochStr + "/";

  if (VERBOSE) {
    char full_path[PATH_MAX];
    realpath(dataFolder.c_str(),full_path);
    string path = full_path;
    outs() << "\n-= Welcome to LLVMVis =-\n"
	   << " - Module title: " << get_name(&m) << "\n"
	   << " - Epoch: " << epochStr << "\n"
	   << " - Output folder: " << path << "\n"
	   << " - Web folder: " << webFolder << "\n"
	   << " - Sync command: " << syncCommand << "\n";
  }
					       

  vector<string> folders; //The folders/views created
  
  //Name all loops. Note: This indexes on the first block of a loop
  //instead of the loop itself, because the latter yields incorrect
  //results 
  int loopID = 0;
  for (Function &f : m) {
    if (f.isDeclaration()) continue;

    LoopInfo *LI = &getAnalysis<LoopInfoWrapperPass>(f).getLoopInfo();
    for (BasicBlock &b : f) {
      
      //Get the loop for this block
      Loop *loop = LI->getLoopFor(&b);

      if (!loop) {
	loopIDs[&b] = "";
  	continue;
      }
      
      //Get/set the loop number
      if (loopNum.find(loop->getBlocks().front()) == loopNum.end()) {
	loopNum[loop->getBlocks().front()] = loopID;
	loopID++;
      }

      //Map this block, to the loop ID. This will put associate a
      //block to its inner most loop      
      loopIDs[&b] = to_string(loopNum[loop->getBlocks().front()]);
           
    }
  }

  //Create the module control flow graph folder
  if (CREATE_CF_MODULE_VIEW) {
    string name = "Module_Control_" + get_name(&m);;
    string command = "mkdir -p " + dataFolder + name;
    system(command.c_str());
    folders.push_back(name);
  }

  //Create the function level control flow graph folders  
  if (CREATE_CF_FUNCTION_VIEWS) {
    for (Function &f : m) {
      if (f.isDeclaration()) continue;
      if (onlyDoFuns != "all" &&
	  onlyDoFuns.find(get_name(&f)) == string::npos)
      	continue;
	
      string name = "Function_Control_" + get_name(&f);
      string command = "mkdir -p " + dataFolder + name;
      system(command.c_str());
      folders.push_back(name);
    }
  }
  
  //Create the function level data flow graph folders
  if (CREATE_DF_FUNCTION_VIEWS) {
    for (Function &f : m) {
      if (f.isDeclaration()) continue;

      if (onlyDoFuns != "all" &&
	  onlyDoFuns.find(get_name(&f)) == string::npos)
      	continue;
      
      string name = "Function_Data_" + get_name(&f);
      string command = "mkdir -p " + dataFolder + name;
      system(command.c_str());
      folders.push_back(name);
    }
  }

  //Create the control flow view for the module. Functions are nodes,
  //with calls connecting the nodes.
  if (CREATE_CF_MODULE_VIEW) {
    create_control_flow_view(m,folders);
  }

  //Create the control flow view. Each basic block is a node with the
  //branches between the blocks represented as edges in the graph.
  if (CREATE_CF_FUNCTION_VIEWS) {
    for (Function &f : m) {
      if (f.isDeclaration()) continue;
      if (onlyDoFuns != "all" &&
	  onlyDoFuns.find(get_name(&f)) == string::npos)
      	continue;

      
      LoopInfo *LI = &getAnalysis<LoopInfoWrapperPass>(f).getLoopInfo();
      create_control_flow_view(f,folders,LI);
    }
  }

  //Create the data flow view. Arguments + global variables are
  //inputs, and everything with no successor instruction is an output
  if (CREATE_DF_FUNCTION_VIEWS) {
    for (Function &f : m) {
      if (f.isDeclaration()) continue;
      if (onlyDoFuns != "all" &&
	  onlyDoFuns.find(get_name(&f)) == string::npos)
      	continue;
      
      LoopInfo *LI = &getAnalysis<LoopInfoWrapperPass>(f).getLoopInfo();      
      create_data_flow_view(f,folders,LI);


    }
  }

  if (DO_SYNC) {
    if (VERBOSE) outs() << "Running sync command: " << syncCommand << " ... ";

    if (VERBOSE) outs() << "Done\n";
  }

  
  if (VERBOSE) outs() << "-= LLVMVis Complete =-\n\n";
  //This pass did not make changes to the IR
  return false;
}




