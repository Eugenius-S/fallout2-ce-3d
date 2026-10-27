#ifndef FALLOUT_SCENE_SCENE_OBJECTS_H
#define FALLOUT_SCENE_SCENE_OBJECTS_H

#include <vector>

namespace fallout {

struct Object;

namespace scene {

// The objects of the scene's last frame, by pick id, with the hex they were
// on: a pick names one only while it is still there.
struct PickEntry {
    Object* object;
    int tile;
};

struct SceneObjectArt { int fid; int rotation; };
// The visible side (087), shared with emission/light source extraction.
SceneObjectArt shownObjectArt(Object* object);
// Bounded native-art inspection fixtures only; normal objects have no override.
void setObjectArtOffsetForInspection(Object* object, int x, int y);
bool objectArtOffsetForInspection(Object* object, int* x, int* y);

// Every live object on the current elevation into the scene in its current
// frame (critters with their animation, doors open or shut, items where they
// lie), then the hero. True when the hero card shows the hero's frame.
bool updateObjectsFromGame();
// Live objects whose frame could not be drawn in the last update.
unsigned int objectCardsSkipped();
const std::vector<PickEntry>& pickEntries();
// The next unavailable hero frame is logged again.
void resetHeroLog();

// The object the scene shows under a screen pixel, with the engine's rules
// (gameMouseGetObjectUnderCursor). False while the scene is not shown.
bool pickObject(int mouseX, int mouseY, int objectType, bool includeDude, Object** found);

} // namespace scene
} // namespace fallout

#endif /* FALLOUT_SCENE_SCENE_OBJECTS_H */
