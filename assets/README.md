# Longo Doggo runtime assets

The renderer loads the recovered export at runtime. In the repository root it
works without copying anything from:

```text
recovered/exported-assets/
  sprites/<sprite-name>/<sprite-name>_<frame>.png
  audio/audiogroup_default/<sound-name>.wav
```

For a packaged native or web build, copy that directory to
`assets/exported-assets/`, preserving the `sprites/` and
`audio/audiogroup_default/` subdirectories. The renderer searches the
requested root and then `assets/exported-assets`, `assets`,
`recovered/exported-assets`, and their parent-directory equivalents.

The expected recovered resources are the `sprDog*`, `sprDogTail`, `sprBox`,
`sprButton*`, `sprDoor`, `sprHole`, `sprApple`, `sprPear`, `sprSkull`,
`sprHouse`, `sprFlower`, `sprFly`, `sprTitle`, `sprTransition`, `sprSmoke`,
`sprBark`, `sprOne`, `sprOldDog`, and `sprDialogueBox` directories, plus the
seven `snd_*.wav` files. Missing frames or sounds are safe: primitive pixel-art
fallbacks are drawn and unavailable audio is skipped.

For Emscripten, preload the same relative tree into the virtual filesystem,
for example with `--preload-file assets@assets`. The original `recovered/`
tree is never modified by the runtime.
