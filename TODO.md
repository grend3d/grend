A plethora of things to be done
============================================================================================================================================================================================================

#### bugs

- [ ] handle cases where textures, models etc can't be loaded

#### engine stuff

- [x] shader preprocessor
    - [ ] attach shaders to materials and dependently link shaders based on
          what's used in the scene (like the fancy big 3D engines out there)
- [~] shader hot reloading
- [ ] start on physics
- [ ] text rendering
- [ ] quake-style dropdown console for interactive debugging
- [ ] possibly start on a scripting language, think I have some good ideas for
      this but not sure if it's practical to do this over just luaJIT or
      angelscript or scheme or something like that
      (maybe finish up that old JIT scheme and compile to scheme?)
- [ ] 3D sound mixing
- [ ] test framework (after scripting, gltf etc)


#### rendering stuff

- [ ] particles, instanced rendering
- [ ] parallax mapping
- [ ] maybe ambient occlusion
- [ ] anisotropic filtering
