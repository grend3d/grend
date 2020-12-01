ifeq ($(IS_EMSCRIPTEN),1)
DEMO_TARGETS += $(BUILD)/map-viewer.html
DEMO_CLEAN += clean-map-viewer-html

else
DEMO_TARGETS += $(BUILD)/map-viewer
DEMO_CLEAN += clean-map-viewer
endif

$(BUILD)/map-viewer: demos/map-viewer/main.o
	$(CXX) $< $(DEMO_CXXFLAGS) -o $@

$(BUILD)/map-viewer.html: demos/map-viewer/main.o
	$(CXX) $< $(DEMO_CXXFLAGS) $(EM_PRELOAD_FLAGS) -o $@

.PHONY: clean-map-viewer
clean-map-viewer:
	-rm -f demos/map-viewer/main.o
	-rm -f $(BUILD)/demo

.PHONY: clean-map-viewer-html
clean-map-viewer-html:
	-rm -f demos/map-viewer/main.o
	-rm -f $(BUILD)/demo.html
