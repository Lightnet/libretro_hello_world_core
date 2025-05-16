# opengl-cores
- https://docs.libretro.com/development/cores/opengl-cores/
- https://bitbucket.org/Themaister/libretro-gl/src/master/


# Important considerations in the OpenGL code
The frontend and libretro core share OpenGL context state. Some considerations have to be taken into account for this cooperation to work nicely.

- Don’t leave buffers and global objects bound when calling retro_video_refresh_t. Make sure to unbind everything, i.e. VAOs, VBOs, shader programs, textures, etc. Failing to do this could potentially hit strange bugs. The frontend will also follow this rule to avoid clashes. Being tidy here is considered good practice anyway.
- The GL viewport will be modified by frontend as well as libretro core. Set this every frame.
- glEnable() state like depth testing, etc, is likely to be disabled in frontend as it's just rendering a quad to screen. Enable this per-frame if you use depth testing. There is no need to disable this before calling retro_video_refresh_t.
- Avoid VAOs. They tend to break on less-than-stellar drivers, such as AMD drivers on Windows as of 2013
- Try to write code which is GLES2 as well as GL2+ (w/ extensions) compliant. This ensures maximum target surface for the libretro core.
- Libretro treats top-left as origin. OpenGL treats bottom-left as origin. To be compatible with the libretro model, top-left semantics are preserved. Rendering normally will cause the image to be flipped vertically. To avoid this, simply scale the final projection matrix by [1,− 1 , 1 ,1].