#include <exception>
#include <iostream>

#include <grend/glslObject.hpp>
#include <grend/sdlContext.hpp>

using namespace grendx;
using namespace p_comb;

void collectTokens(glslObject& obj, container& tokens);

glslObject::glslObject(std::string_view source) {
	auto tokens = parseGlsl(source);

	if (tokens.empty()) {
		throw std::logic_error("Couldn't parse shader!");
	}

	collectTokens(*this, tokens);
}

void collectTranslationUnit(glslObject& obj, container& tokens);
void collectExternDecl(glslObject& obj, container& tokens);
void collectDecl(glslObject& obj, container& tokens);
void collectFuncDef(glslObject& obj, container& tokens);
void collectDeclInitList(glslObject& obj, container& tokens);
void collectInitDeclContinued(glslObject& obj, container& token);
void collectSingleDeclaration(glslObject& obj, container& token);

void collectExternDecl(glslObject& obj, container& tokens) {
	SDL_Log("collectExternDecl(): got here");
	for (auto tok : tokens) {
		if (tok.tag == "declaration") {
			collectDecl(obj, tok.subtokens);

		} else if (tok.tag == "function_definition") {
			collectFuncDef(obj, tok.subtokens);
		}
	}
}

void collectDecl(glslObject& obj, container& tokens) {
	SDL_Log("collectDecl(): got here");

	for (auto tok : tokens) {
		if (tok.tag == "declaration_init_decl_list") {
			collectDeclInitList(obj, tok.subtokens);
		}
	}
}

void collectDeclInitList(glslObject& obj, container& tokens) {
	SDL_Log("collectDeclInitList(): got here");

	if (tokens.empty()) {
		throw std::logic_error("empty list");
	}

	if (tokens[0].tag != "init_declarator_list") {
		throw std::logic_error("not an init_declarator_list");
	}

	auto sub = tokens[0].subtokens;

	for (auto tok : sub) {
		if (tok.tag == "single_declaration") {
			collectSingleDeclaration(obj, tok.subtokens);
			dump_tokens_tree(tok.subtokens);

		// TODO: should have picked a better tag for this lols
		} else if (tok.tag == "init_declarator_asdf") {
			collectInitDeclContinued(obj, tok.subtokens);
			dump_tokens_tree(tok.subtokens);
		}
	}
}

void collectSingleDeclaration(glslObject& obj, container& token) {
	SDL_Log("collectSingleDeclaration(): got here");
}

void collectInitDeclContinued(glslObject& obj, container& token) {
	SDL_Log("collectInitDeclContinued(): got here");
}

void collectFuncDef(glslObject& obj, container& tokens) {
	SDL_Log("collectFuncDef(): got here");
}

void collectTranslationUnit(glslObject& obj, container& tokens) {
	SDL_Log("collectTranslationUnit(): got here");
	for (auto tok : tokens) {
		std::cout << tok.tag << std::endl;
		if (tok.tag == "external_declaration") {
			collectExternDecl(obj, tok.subtokens);
		}
	}
}

void collectTokens(glslObject& obj, container& tokens) {
	SDL_Log("collectTokens(): got here");
	for (auto tok : tokens) {
		if (tok.tag == "translation_unit") {
			collectTranslationUnit(obj, tok.subtokens);
		}
	}
}
