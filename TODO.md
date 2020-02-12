Adding a todo because the number of things to do is expanding exponentially and indefinitely, bounded only by my finite lifespan and the number of possible contributors and it gets hard to remember it all
============================================================================================================================================================================================================

#### engine stuff

- [ ] glTF loader (not necessary, could use existing lib if it's too much to do)
- [ ] shader preprocessor
    - [ ] graceful fallback to older versions of glsl, eg. for embedded,
          mobile, web stuff
    - [ ] attach shaders to materials and dependently link shaders based on
          what's used in the scene (like the fancy big 3D engines out there)
- [ ] shader hot reloading
- [ ] start on physics
- [ ] scene object selection
- [ ] text rendering
- [ ] other 2D stuff, having good 2D primitives can't hurt
- [ ] quake-style dropdown console for interactive debugging
- [ ] possibly start on a scripting language, think I have some good ideas for
      this but not sure if it's practical to do this over just luaJIT or
      angelscript or scheme or something like that
      (maybe finish up that old JIT scheme and compile to scheme?)
- [ ] 3D sound mixing
- [ ] test framework (after scripting, gltf etc)


#### rendering stuff

- [ ] more optimization on the opengl side, uniform buffer objects
      (eg for light, material info)
- [ ] PBR lighting
- [ ] particles, instanced rendering
- [ ] shadows
- [ ] cube maps
- [ ] reflections
- [ ] parallax mapping
- [ ] maybe ambient occlusion
- [ ] anisotropic filtering
- [ ] deferred shading


#### meta

- [ ] learn me some 3D modeling and drawing
- [ ] start on a real demo game for the engine
