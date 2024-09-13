# Usage

If you have your own sensor, on your own microcontroller,
with your own firmware,
and you're wondering what would be required to make your device compatible with Bristlemouth,
read on!

## Build System Integration
Bristlemouth heavily relies on being built with CMake.
In order to integrate it within your own project,
CMake is highly recommended.
The following explains how to integrate Bristlemouth into your own project utilizing CMake:

```
# Show how to integrate with CMake here
```

<!--- Explain any info pertaining to CMake integration down here --->


Bristlemouth can be integrated into other build systems by...

<!--- Show how other build systems can utilize bm--->

## OS Support
Bristlemouth is designed to be platform agnostic.
However, it currently relies on certain OS features to function properly.

<!-- Show how to use one of the available platforms (starting with FreeRTOS)--->

## Running The Bristlemouth Stack
Once Bristlemouth has been added to the project,
it can be integrated within the project by configuring the following components:

<!--- TODO: Refine this--->
- Network Interface
- Bristlemouth

These items are configured by calling their initialization functions.
An example of this and how the Bristlemouth stack is to be serviced is shown below:

```C
Adin2111 adin;
Bristlemouth bm;

int main() {
    // other sensor setup
    adin->init(...pins...spi peripheral...);
    bm->init(adin, config);
    for(;;) {
        // other sensor code
        bm->update();
    }
}
```

<!--- Explain function parameters here?--->
