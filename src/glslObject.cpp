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

// XXX
std::string collectStrs(container& tokens) {
	std::string ret = "";

	for (auto tok : tokens) {
		ret += collect(tok.subtokens);

		// handle base characters
		if (tok.tag.size() == 0) {
			ret += tok.data;
		}
	}

	return ret;
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

	auto& sub = tokens[0].subtokens;

	if (sub[0].tag != "single_declaration") {
		throw std::logic_error("expected a single_declaration");
	}

	auto& firstdec = sub[0].subtokens;
	auto& typespec = firstdec[0].subtokens;

	SDL_Log("have type specifier: ");
	dump_tokens_tree(typespec);

	for (auto tok : sub) {
		if (tok.tag == "single_declaration") {
			collectSingleDeclaration(obj, tok.subtokens);
			//dump_tokens_tree(tok.subtokens);

		// TODO: should have picked a better tag for this lols
		} else if (tok.tag == "init_declarator_asdf") {
			collectInitDeclContinued(obj, tok.subtokens);
			//dump_tokens_tree(tok.subtokens);
		}
	}
}

void collectSingleDeclaration(glslObject& obj, container& tokens) {
	SDL_Log("collectSingleDeclaration(): got here");
	if (tokens.size() < 2) {
		SDL_Log("Have declaration with no name? ignoring");
		return;
	}

	auto& typespec   = tokens[0];
	auto& identifier = tokens[1];

	std::string name = collectStrs(identifier.subtokens);
	SDL_Log("Have declaration with name: '%s'", name.c_str());
}

void collectInitDeclContinued(glslObject& obj, container& tokens) {
	SDL_Log("collectInitDeclContinued(): got here");

	if (tokens.size() < 2) {
		// TODO: rename
		throw std::logic_error("init_declarator_asdf is too short!");
	}

	auto& identifier = tokens[1];
	std::string name = collectStrs(identifier.subtokens);
	SDL_Log("Have another declaration with name: '%s'", name.c_str());
}

void collectFuncDef(glslObject& obj, container& tokens) {
	SDL_Log("collectFuncDef(): got here");

	if (tokens.size() < 2) {
		throw std::logic_error("function definition is too small!");
	}

	auto& prototype = tokens[0].subtokens;
	auto& body      = tokens[1].subtokens;

	dump_tokens_tree(prototype);
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
