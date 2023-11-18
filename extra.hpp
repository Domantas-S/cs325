#ifndef EXTRA_HPP
#define EXTRA_HPP

#include <string>
#include "llvm/IR/Value.h"

class ASTnode {
public:
  virtual ~ASTnode() {};
  virtual llvm::Value *codegen() = 0;
  virtual std::string to_string() const;
  virtual std::string to_tree(std::string prefix, bool end) const = 0;
};

#endif