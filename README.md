<p align="left">
<img width="418" height="190" alt="image" src="https://github.com/user-attachments/assets/23fd3085-6673-4e03-9d36-0adf7a4645e2" />
</p>



## What on earth is this?
**furious** is a specialized video editor and effects engine for creating YTPMVs/otomads and other music-focused content. It's basically a video editor, but the entire project is based on a BPM similar to a DAW. Doing this allows for easily syncing clips up to supporting audio and music. You no longer
have to worry about timing or any other nonsense like that.

<img width="600" height="500" alt="image" src="https://github.com/user-attachments/assets/37d3c7b7-e43f-4e9f-916c-d937ba2f943d" />

### Custom video engine
furious uses a custom video engine built on FFmpeg decoding libraries. The goal is for playback
to be as uninterrupted and smooth as possible, while being able to handle a bunch of sources on screen at once. 

<img width="437" height="361" alt="image" src="demo.gif" />

### Keyframing & Automation
furious comes with a "Patterns" system similar to FL studio. You can use this to tailor your clips to sync to certain sections or measures of your background music. 

<img width="537" height="361" alt="image" src="https://github.com/user-attachments/assets/978d8554-255a-455d-aed5-a32ff235a5aa" />


### Lua scripting
furious comes with an effects engine, with **Lua scripting support!** All the clip effects are loaded from the effect scripts folder at runtime. Right now Lua is used mainly for effects, but I eventually
want to let it do **anything** in the application.

<img width="700" height="600" alt="image" src="https://github.com/user-attachments/assets/159f7f05-ed2c-491f-acba-2f2114cedbba" />


### Development Warning
furious is in **very early development.** There are a lot of issues that I'm aware of, and a ton more that I'm not aware of. A lot of things may need to be reworked or changed completely. The current state of FURIOUS is purely a proof of concept and not intended to be a final product.

## Building
```bash
mkdir build
cd build
cmake ..
make
```

## Roadmap
Here are some things I'm planning to add, or are still in the cooker:
- Timeline rendering/export
- Timeline envelope automation
- More more and more effects
- Fully featured source library/clips/timeline functionality, like what you'd expect from modern editing software
- Media Proxies
- Testing and functionality on more gpus, more video codecs, and more operating systems
- Vulkan backend

# License
MIT
