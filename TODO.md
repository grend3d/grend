### General
- work on building and streaming mesh data in real time
- add a marching cubes implementation
- start merging in nscheme stuff
- handle controllers in scancode header
- split editor into seperate library
- come up with a better name for the "scancode" header
- maybe remove gameObject from entities
  and have only an entity transform (or even have that as a component)
- less OOP in ECS, it was a worthy experiment but a more typical data-oriented
  approach seems to be the way to go
- fast slab allocators in ECS, can map allocators using type IDs
  (should be very similar to a kernel memory allocator)
- dependency resolver for ECS systems to automatically order system execution,
  determine which systems can be run in parallel
- (more) graphics API abtraction, start preparing for vulkan
- more physics stuff, constraints, sphere collision checks
- generic particle system, optionally renderable
- start on a netcode prototype using ECS stuff
- handle window resizing (again)

### UI
- Automatically scroll to selected entity in the list when a new entity
  is selected in the editor
- make a prototype of UI layout on top of imgui/nuklear
- completely phase out modalSDLInput, need a better way of abstracting
  away input events, polling input rather than passing around events,
  providing a clean way to remap bindings
- have a convenient way to render text, UI stuff into textures to be
  displayed in the world
- Automatic tool for placing, baking light volumes

### Rendering
- Compute shader to determine average brightness, automatically adjust exposure
- Specular convolution for reflection probes
- SSAO
- SSR
- render world into seperate framebuffer in editor so that docking stuff
  actually makes sense
- Plain old irradiance volumes, thinking this could be stored as
  a json object + PNG, including positions (not necessarily a grid),
  then the info can be copied to the main irradiance atlas as needed
- Compress irradiance info, either HL2-style or spherical harmonics
- BVH for fast probe lookup in shaders, buffer irradiance probes to GPU
- Implement some way of determining which G-buffer layers shaders depend on,
  so only what is needed will be stored

### Experimental
- Compute shader to bin lights
- Store delta transforms from last frame in buffer indexed by mesh ID,
  store mesh IDs in framebuffer attachment
  (this would be used to derive velocities, adjust surfel positions in a surfel GI)
- TSSAA
- Raytracing surfel GI
- Variation of PRT GI used in that one talk, stored similarly to plain
  irradiance volumes, but with the G-buffers in seperate PNGs
