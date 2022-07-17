#include <p_comb/parser.hpp>
#include <p_comb/ebnfish.hpp>
#include <string_view>

using namespace p_comb;

namespace grendx {

cparser loadGlslParser(void);
container parseGlsl(std::string_view source);

// namespace grendx
}
