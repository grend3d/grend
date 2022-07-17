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
			if (auto sub = tok.subtokens) {
				collectDecl(obj, *sub);
			} else throw std::logic_error("no subtokens");

		} else if (tok.tag == "function_definition") {
			if (auto sub = tok.subtokens) {
				collectFuncDef(obj, *sub);
			} else throw std::logic_error("no subtokens");
		}
	}
}

void collectDecl(glslObject& obj, container& tokens) {
	SDL_Log("collectDecl(): got here");

	for (auto tok : tokens) {
		if (tok.tag == "declaration_init_decl_list") {
			if (auto sub = tok.subtokens) {
				collectDeclInitList(obj, *sub);
			} else throw std::logic_error("no subtokens");
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

	auto sub = *tokens[0].subtokens;

	for (auto tok : sub) {
		if (tok.tag == "single_declaration") {
			if (auto sub = tok.subtokens) {
				collectSingleDeclaration(obj, *sub);
				dump_tokens_tree(*sub);
			} else throw std::logic_error("no subtokens");

		// TODO: should have picked a better tag for this lols
		} else if (tok.tag == "init_declarator_asdf") {
			if (auto sub = tok.subtokens) {
				collectInitDeclContinued(obj, *sub);
				dump_tokens_tree(*sub);
			} else throw std::logic_error("no subtokens");
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
			if (auto sub = tok.subtokens) {
				collectExternDecl(obj, *sub);
			} else throw std::logic_error("no subtokens");
		}
	}
}

void collectTokens(glslObject& obj, container& tokens) {
	SDL_Log("collectTokens(): got here");
	for (auto tok : tokens) {
		if (tok.tag == "translation_unit") {
			if (auto sub = tok.subtokens) {
				collectTranslationUnit(obj, *sub);
			} else throw std::logic_error("no subtokens");
		}
	}
}
