- handle controllers in scancode header
- come up with a better name for the "scancode" header
- clean up editor file
- split editor into seperate library
- start merging in nscheme stuff
- completely phase out modalSDLInput, need a better way of abstracting
  away input events, polling input rather than passing around events,
  providing a clean way to remap bindings
- rework clicking code, maybe pass a click ID to renderQueue::add,
  use seperate R16/R32 attachment so stencil buffer is free for other
  purposes
- remove onClick(), etc functions from gameObject, should have callbacks
  mapped from click IDs
- rename gameObject to sceneNode or something
- consider how material editing and overrides will work 
- render world into seperate framebuffer in editor so that docking stuff
  actually makes sense
- make a prototype of UI layout on top of imgui/nuklear
- have a convenient way to render text, UI stuff into textures to be
  displayed in the world
- more physics stuff, constraints, sphere collision checks
- generic particle system, optionally renderable
- work on building and streaming mesh data in real time
- add a marching cubes implementation
- clean up sceneTree ECS components, perhaps remove gameObject from entities
  and have only an entity transform (or even have that as a component)
- less OOP in ECS, it was a worthy experiment but a more typical data-oriented
  approach seems to be the way to go
- fast slab allocators in ECS, can map allocators using type IDs
  (should be very similar to a kernel memory allocator)
- dependency resolver for ECS systems to automatically order system execution,
  determine which systems can be run in parallel
- (more) graphics API abtraction, start preparing for vulkan
- start on a netcode prototype using ECS stuff
