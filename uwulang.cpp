/* 
* UwULang, an esoteric programming language with emojis
* More info on https://uwulang.com
* Version 0.1.1 
* License: MIT
*/ 

#include <iostream>
#include <string>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <argp.h>
#include "llvm/ADT/Optional.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"

// program version
#define VERSION         "0.1.1"

static char DOC[] = "UwULang, an esoteric programming language with emojis";
const char *argp_program_version = VERSION;

static llvm::LLVMContext Context;
static llvm:: IRBuilder<> Builder(Context);
static std::unique_ptr<llvm:Module> Module; // so we don't have to free it

/**
 * Moves current block.
 *
 * When we need to jump to the next or previous block on the tape, we
 * need to emit a jump instruction.
 *
 * @param ptr Pointer to the current block.
 * @param amount Amount of block to move (-1 or +1)
 * @return 
 */
void emit_move_ptr(llvm::Value* ptr, int amount) {
    Builder.CreateStore(
        Builder.CreateGEP(
            Builder.getInt8Ty(),
            Builder.CreateLoad(ptr),
            Builder.getInt32(amount)
        ),
        ptr
    )
}

/**
 * Increments the value of the current block.
 * 
 * @param ptr Pointer to the current block.
 * @param amount Amount to increment the value by (-1 or +1)
 * @return
*/
void emit_value(llvm::Value* ptr, int amount) {
    // get the value of the current block
    llvm::Value* val = Builder.CreateLoad(ptr);
    Builder.CreateStore(
        Builder.CreateAdd(
            Builder.CreateLoad(val),
            Builder.getInt8(amount)
        ),
        val
    );
}

/* IO sys calls */
/**
 * Prints the current block.
 * 
 * @param ptr Pointer to the current block.
 * @return
*/
void emit_print(llvm::Value* ptr) {
    llvm::Function* printChar = llvm::cast<llvm::Function>(
        Module->getOrInsertFunction(
            "putchar",
            Builder.getInt32Ty(),
            Builder.getInt32Ty(),
            nullptr
        )
    );
    // sys call to print the current block
    Builder.CreateCall(
        printChar,
        Builder.CreateSExt(
            Builder.CreateLoad(ptr),
            Builder.getInt32Ty()
        )
    );
}

/**
 * Reads a char from stdin and stores it in the current block.
 * 
 * @param ptr Pointer to the current block.
 * @return
*/
void emit_read(llvm::Value* ptr) {
    llvm::Function* readChar = llvm::cast<llvm::Function>(
        Module->getOrInsertFunction(
            "getchar",
            Builder.getInt32Ty(),
            nullptr
        )
    );
    // sys call to read a char from stdin
    Builder.CreateStore(
        Builder.CreateTrunc(
            Builder.CreateCall(readChar),
            Builder.getInt8Ty()
        ),
        ptr
    );
}

/* loops - think of loop varients as while (curr) */
struct WhileBlock {
    llvm::BasicBlock* cond_block;
    llvm::BasicBlock* body_block;
    llvm::BasicBlock* end_block;
};

/**
 * Emits the beginning of a while loop.
 * 
 * @param func Function to emit the loop in.
 * @param ptr Pointer to the current block.
 * @param while_block Pointer to the while block struct.
 * @param while_index Index of the while loop.
 * @return
*/
void emit_while_begin(llvm::Function* func, llvm::Value* ptr, WhileBlock* while_block, int while_index) {
    while_block->cond_block = llvm::BasicBlock::Create(
        Context, std::string("while_cond") + std::to_string(while_index), func);
    while_block->body_block = llvm::BasicBlock::Create(
        Context, std::string("while_body") + std::to_string(while_index), func);
    while_block->end_block = llvm::BasicBlock::Create(
        Context, std::string("while_end") + std::to_string(while_index), func);
    Builder.CreateBr(while_block->cond_block);
    Builder.SetInsertPoint(while_block->cond_block);
    Builder.CreateCondBr(
        Builder.CreateICmpNE(
            Builder.CreateLoad(Builder.CreateLoad(ptr)),
            Builder.getInt8(0)),
        while_block->body_block,
        while_block->end_block);
    Builder.SetInsertPoint(while_block->body_block);
}

/**
 * Emits the end of a while loop.
 * 
 * @param while_block Pointer to the while block struct.
 * @return
*/
void emit_while_end(WhileBlock* while_block) {
    Builder.CreateBr(while_block->cond_block);
    Builder.SetInsertPoint(while_block->end_block);
}

int compile_uwulang(int charCounter, unsigned char code[]) {
    // initalization of LLVM
    Module = llvm::make_unique<llvm::Module>("top", Context);
    llvm::Function* mainFunc = llvm::Function::Create(
        llvm::FunctionType::get(llvm::Type::getInt32Ty(Context), false),
        llvm::Function::ExternalLinkage, "main", Module.get());
    Builder.SetInsertPoint(llvm::BasicBlock::Create(Context, "", mainFunc));

    llvm::Value* data = Builder.CreateAlloca(Builder.getInt8PtrTy(), nullptr, "data");
    llvm::Value* ptr = Builder.CreateAlloca(Builder.getInt8PtrTy(), nullptr, "ptr");
    llvm::Function* funcCalloc = llvm::cast<llvm::Function>(
        Module->getOrInsertFunction("calloc",
            Builder.getInt8PtrTy(),
            Builder.getInt64Ty(), Builder.getInt64Ty(),
            nullptr));
    llvm::Value* data_ptr = Builder.CreateCall(funcCalloc, {Builder.getInt64(30000), Builder.getInt64(1)});
    Builder.CreateStore(data_ptr, data);
    Builder.CreateStore(data_ptr, ptr);

    int while_index = 0;
    WhileBlock while_blocks[1000];
    WhileBlock* while_block_ptr = while_blocks;
    // get the input
    // TODO: 
    int _ptr = 0;
    int codePtr = 0;
    int currChar, nextChar;
    while (code[_ptr] != EOF && code[_ptr] != '\0') {
        // if not emoji continue
        if (code[_ptr] != 240) {
            _ptr++;
            continue;
        }
        _ptr++;
        if (code[_ptr] != 159) {
            ptr++;
            continue;
        }
        _ptr++;

        currChar = code[_ptr];
        nextChar = code[_ptr+1];

        if (currChar == 145 && nextChar == 134) {
            emit_value(ptr, 1);
        }
        else if (currChar == 145 && nextChar == 135) {
            emit_value(ptr, -1);
        }
        else if (currChar == 165 && nextChar == 186) {
            emit_print(ptr);
        }
        else if (currChar == 152 && nextChar == 179) 
            emit_read(ptr);
        else if (currChar == 145 && nextChar == 137) 
            emit_move_ptr(ptr, 1);
        else if (currChar == 145 && nextChar == 136) {
            emit_move_ptr(ptr, -1);
        } else if (currChar == 152 && nextChar == 146) {
            emit_while_begin(mainFunc, ptr, while_block_ptr++, while_index++);
        } else if (currChar == 152 && nextChar == 161) {
            // 😡 ]
            if (--while_block_ptr < while_blocks) {
                std::cerr << "unmatching 😡\n";
                return 1;
            }
        }

        ptr+=2;
        codePtr++;
    }

    // build the AST

    llvm::Function* funcFree = llvm::cast<llvm::Function>(
        Module->getOrInsertFunction("free",
            Builder.getVoidTy(),
            Builder.getInt8PtrTy(),
            nullptr));
    Builder.CreateCall(funcFree, {Builder.CreateLoad(data)});

    Builder.CreateRet(Builder.getInt32(0));

    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    std::string TargetTriple = llvm::sys::getDefaultTargetTriple();

    std::string err;
    const llvm::Target* Target = llvm::TargetRegistry::lookupTarget(TargetTriple, err);
    if (!Target) {
        std::cerr << "Failed to lookup target " + TargetTriple + ": " + err;
        return 1;
    }

    llvm::TargetOptions opt;
    llvm::TargetMachine* TheTargetMachine = Target->createTargetMachine(
        TargetTriple, "generic", "", opt, llvm::Optional<llvm::Reloc::Model>());

    Module->setTargetTriple(TargetTriple);
    Module->setDataLayout(TheTargetMachine->createDataLayout());

    std::string Filename = "output.o";
    std::error_code err_code;
    llvm::raw_fd_ostream dest(Filename, err_code, llvm::sys::fs::OF_None);
    if (err_code) {
        std::cerr << "Could not open file: " << err_code.message();
        return 1;
    }

    llvm::legacy::PassManager pass;
    if (TheTargetMachine->addPassesToEmitFile(pass, dest, llvm::TargetMachine::CGFT_ObjectFile)) {
        std::cerr << "TheTargetMachine can't emit a file of this type\n";
        return 1;
    }
    pass.run(*Module);
    dest.flush();
    std::cout << "Wrote " << Filename << "\n";

    return 0;
}

int file_entry(char* filename) {
    // inital way
    unsigned char code[32768] = {0};

    int charCounter = 0;
    FILE *fp;
    // open file
    fp = fopen(filename, "r");
    if (!fp) {
        printf("Error in opening file!\n");
        return 1;
    }
    fscanf(fp, "%[^=]=%[^;]; ", code);
    charCounter = strlen((char*)code);
    fclose(fp);
    compile_uwulang(charCounter, code);
    return 0;
}

static int parse_opt(int key, char *arg, struct argp_state *state) {
    switch (key) {
        case 'v': 
            printf("UwULang %s\n", VERSION);
            break;
        // get from file
        case 'f':
        case 'l':
            // print arg
            // printf("File: %s\n", arg);
            if (file_entry(arg)) {
                return 1;
            }
            break;
        case ARGP_KEY_ARG:
            if (state->arg_num == 0) {
                // First argument is provided
                file_entry(arg);
            } else {
                // Too many arguments
                printf("Too many arguments provided\n");
            }
            break;
        // case ARGP_KEY_END:
        //     if (state->arg_num < 1) {
        //         // No arguments provided, use default
        //         cin_entry();
        //     }
        //     break;
    }
    return 0;
}

/// Main takes in either a single file with file extension uwu
/// or if no args are present will run interpreted on the command line
int main(int argc, char *argv[]) {
    // no buffer when printing
    setbuf(stdout, NULL);

    struct argp_option options[] = {
        { "file", 'f', "FILE", 0, ".uwu input file"},
        { "filename", 'l', "FILE", 0, ""},
        { 0 }
    };
    struct argp argp = { options, parse_opt, 0, DOC };
    return argp_parse(&argp, argc, argv, 0, 0, 0); 
}
