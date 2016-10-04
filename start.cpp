#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Constants.h"
#include <vector>
#include <string>
#include <stdio.h>
#include <iostream>

int main()
{
  llvm::LLVMContext context;
  llvm::Module *module = new llvm::Module("amdgpu", context);
  llvm::IRBuilder<> builder(context); 
  module->dump();
}
