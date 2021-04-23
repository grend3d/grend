#include <grend/renderPostStage.hpp>

using namespace grendx;

// non-pure virtual destructors for rtti
// TODO: maybe have seperate source for postprocessing stuff
rUninitialized::~rUninitialized() {};
rOutput::~rOutput() {};
rStage::~rStage() {};

