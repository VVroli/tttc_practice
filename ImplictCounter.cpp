#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

namespace {

class CastVisitor : public RecursiveASTVisitor<CastVisitor> {
public:
    explicit CastVisitor(ASTContext *Context) : Context(Context) {}

    bool VisitImplicitCastExpr(ImplicitCastExpr *ICE) {
        if (!ICE->getBeginLoc().isValid()) return true; 
        QualType FromType = ICE->getSubExpr()->getType();
        QualType ToType = ICE->getType();

        std::string FromStr = FromType.getAsString();
        std::string ToStr = ToType.getAsString();

    
        CurrentFunctionCasts[FromStr + " -> " + ToStr]++;
        return true;
    }

    void SetCurrentFunction(const FunctionDecl *FD) {
        CurrentFunction = FD;
        CurrentFunctionCasts.clear();
    }

    void PrintAndResetCounts() {
        if (!CurrentFunction || CurrentFunctionCasts.empty()) return;

        llvm::outs() << "Function `" << CurrentFunction->getName() << "`\n";
        for (const auto &Pair : CurrentFunctionCasts) {
            llvm::outs() << Pair.first << ": " << Pair.second << "\n";
        }
        llvm::outs() << "\n";
    }

private:
    ASTContext *Context;
    const FunctionDecl *CurrentFunction = nullptr;
    std::map<std::string, int> CurrentFunctionCasts;
};

class CastCounterConsumer : public ASTConsumer {
public:
    explicit CastCounterConsumer(ASTContext *Context) : Visitor(Context) {}

    void HandleTranslationUnit(ASTContext &Context) override {
        TraverseDecls(Context.getTranslationUnitDecl());
    }

    void TraverseDecls(Decl *D) {
        if (!D) return;

        if (auto *FD = dyn_cast<FunctionDecl>(D)) {
            Visitor.SetCurrentFunction(FD);
            Visitor.TraverseDecl(D);
            Visitor.PrintAndResetCounts();
        }
   
        if (auto *DC = dyn_cast<DeclContext>(D)) {
            for (auto *Child : DC->decls()) {
                TraverseDecls(Child);
            }
        }
    }

private:
    CastVisitor Visitor;
};

class CastCounterAction : public PluginASTAction {
protected:
    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                  llvm::StringRef) override {
        return std::make_unique<CastCounterConsumer>(&CI.getASTContext());
    }

    bool ParseArgs(const CompilerInstance &CI,
                  const std::vector<std::string> &args) override {
        return true;
    }
};

} 
static FrontendPluginRegistry::Add<CastCounterAction>
    X("implicit-cast-counter", "Count implicit type conversions");
