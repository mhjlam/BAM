This is Blood & Magic Plus, or 2.0, or whatever.

The biggest change is the porting to modern linux, using SDL3.

It's mostly drop-in compatible with the old executable, except saves and meta-progression.

In addition to this, there are new gameplay features:
- Speed setting (normal, fast, fastest - 100%, 150% and 200%)
- Control groups
- A bit more precise unit selection (less "funky" cursor selection)
- Selecting additional units by holding "shift"
- Option to show health bars below units when damaged
- Option to enable faster auto-collection of mana (instead of collecting after "20" mana but sending 10, send after collecting 10)

And some other under the hood changes:
- Saves and config are written to ~/.local/share/TachyonStudios/BloodAndMagic
- Saves are now json files, so more easily inspected and edited.
- A lot of code modernization to make it a more "native" modern game.

Multiplayer works over LAN/internet via ENet UDP:
- Host: `./bam -NET host` (or `-NET host:<port>`)
- Client: `./bam -NET <ip>` (or `-NET <ip>:<port>`)
- Default port: 7733
- Host's game speed and fast-collect settings are applied to both players


All of the code work for this has been done by an LLM, because I don't know much C++, and I definitely do not know DOS-era C++ (and I don't want to learn it either). I just wanted to play some BAM with modern conveniences.
