#include"llvm/ADT/ArrayRef.h"
#include"llvm/IR/LLVMContext.h"
#include"llvm/IR/Module.h"
#include"llvm/IR/Function.h"
#include"llvm/IR/BasicBlock.h"
#include"llvm/IR/IRBuilder.h"
#include"llvm/IR/Constants.h"
#include<iostream>
#include<vector>
#include<string>
#include<stdint.h>
#include<memory>
#include<map>
#include<fstream>

llvm::LLVMContext context;
llvm::Module *module = new llvm::Module("top", context);
llvm::IRBuilder<> builder(context);

std::map<std::string, llvm::Value*> NamedValues;

std::string funcStr = "def add(a, b, c) c = a + b; c = a - b;";
std::string defStr = "def add() def sub()";

void generateStmt(std::string Stmt)
{
  uint32_t iter = 0;
  uint32_t size = Stmt.size();
  std::vector<std::string> names;
  std::string name;
  std::string rhsName, lhsName, assignName;
  uint32_t op;
  while(iter != size)
  {
    if(Stmt[iter] == ' '){ }
    else{
      if((Stmt[iter] == '+') | (Stmt[iter] == '-') | (Stmt[iter] == '*') | (Stmt[iter] == '/')){
        names.push_back(name);
        op = Stmt[iter];
        lhsName = name;
        name.clear();
      }else{
        if(Stmt[iter] == '='){
          names.push_back(name);
          assignName = name;
          name.clear();
        }else{
          name.push_back(Stmt[iter]);
        }
      }
    }
    iter++;
  }
  rhsName = name;

  NamedValues[rhsName + "1"] = builder.CreateLoad(NamedValues[rhsName], rhsName);
  NamedValues[lhsName + "1"] = builder.CreateLoad(NamedValues[lhsName], lhsName);

  llvm::Value *opValue;

  switch(op) {
    case '+':
      opValue = builder.CreateFAdd(NamedValues[lhsName + "1"], NamedValues[rhsName + "1"], "addtmp");
      NamedValues[opValue->getName()] = opValue;
      break;
    case '-':
      opValue = builder.CreateFSub(NamedValues[lhsName + "1"], NamedValues[rhsName + "1"], "subtmp");
      NamedValues[opValue->getName()] = opValue;
      break;
    case '*':
      opValue = builder.CreateFMul(NamedValues[lhsName + "1"], NamedValues[rhsName + "1"], "multmp");
      NamedValues[opValue->getName()] = opValue;
      break;
    case '/':
      opValue = builder.CreateFDiv(NamedValues[lhsName + "1"], NamedValues[rhsName + "1"], "divtmp");
      NamedValues[opValue->getName()] = opValue;
      break;
  }

  builder.CreateStore(opValue, NamedValues[assignName]);

  names.push_back(name);
#ifdef DEBUG
  std::cout<<names.size()<<std::endl;
  for(uint32_t i=0;i<names.size();i++)
  {
    std::cout<<names[i]<<std::endl;
  }
#endif
}

void generateBodyAST(std::string body)
{
  std::vector<std::string> Stmts;
  std::string Stmt = body;
  while(Stmt.find(";") != std::string::npos)
  {
    Stmts.push_back(Stmt.substr(0, Stmt.find(";")));
    Stmt = Stmt.substr(Stmt.find(";")+1);
  }
  for(uint32_t i=0;i<Stmts.size();i++)
  {
  #ifdef DEBUG
    std::cout<<Stmts[i]<<std::endl;
  #endif
    generateStmt(Stmts[i]);
  }
}

void generateFuncAST(std::string &func, uint32_t defPos){
  uint32_t iter = defPos;
  std::string funcName;
  while(func[iter] != '('){
    if(func[iter] == ' '){}
    else{ funcName.push_back(func[iter]); }
    iter++;
  }
  iter++;
  std::vector<std::string> Args;
  std::string Arg;

  while(func[iter] != ')'){
    if(func[iter] == ' '){}
    else{
      if(func[iter] == ','){ Args.push_back(Arg); Arg.clear(); }
      else{ Arg.push_back(func[iter]); }
    }
    iter++;
  }
  iter++;
  Args.push_back(Arg);

  std::vector<llvm::Type*> argPointers(Args.size());

  for(uint32_t i=0;i<Args.size();i++){
#ifdef DEBUG
    std::cout<<funcName<<std::endl;
    std::cout<<Args[i]<<std::endl;
#endif
    argPointers[i] = llvm::PointerType::get(builder.getFloatTy(), 1);
  }

  llvm::FunctionType *funcType = llvm::FunctionType::get(builder.getVoidTy(), argPointers, false);

  llvm::Function *mainFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, funcName.c_str(), module);

  mainFunc->setCallingConv(llvm::CallingConv::SPIR_KERNEL);

  llvm::BasicBlock *entry = llvm::BasicBlock::Create(context, "", mainFunc);
  builder.SetInsertPoint(entry);

  uint32_t Idx = 0;
  for(auto &Arg: mainFunc->args()){
    std::string str = Args[Idx++] + std::string("_ptr");
    Arg.setName(str);
    NamedValues[str] = &Arg;
  }

  std::vector<llvm::Type*> workItemArgs;

  llvm::FunctionType *workItemType =
    llvm::FunctionType::get(llvm::Type::getInt32Ty(context), workItemArgs, false);
  llvm::Function::Create(workItemType, llvm::Function::ExternalLinkage, "llvm.amdgcn.workitem.id.x", module);

  llvm::Function *workItem = module->getFunction("llvm.amdgcn.workitem.id.x");
  std::vector<llvm::Value*> workItemArgsValue;
  llvm::Value* workItemPtr = builder.CreateCall(workItem, workItemArgsValue, "tid");

  std::vector<llvm::Value*> argsGEP(1, workItemPtr);
  std::vector<llvm::Value*> GEP;

  for(Idx=0;Idx < Args.size(); Idx++){
    std::string _ptr = Args[Idx] + std::string("_ptr");
    llvm::Value *ptr = builder.CreateGEP(NamedValues[_ptr], argsGEP, llvm::Twine(Args[Idx]));
    NamedValues[Args[Idx]] = ptr;
    GEP.push_back(ptr);
  }

  generateBodyAST(func.substr(iter));
  builder.CreateRetVoid();

  module->dump();
}

int main(){
  std::cout<<"; Generating AMDGPU LLVM IR"<<std::endl;
  uint32_t defPos = funcStr.find("def");
  generateFuncAST(funcStr, defPos + 3);
}
