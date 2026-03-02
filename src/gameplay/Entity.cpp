#include "gameplay/Entity.h"
#include "core/Constants.h"

namespace LightsOut {

Entity::Entity(uint32_t tags_) : tags(tags_) {}

bool Entity::isOffscreen(float cameraX, float renderWidth) const {
    return (position.x + width < cameraX - 16.0f) ||
           (position.x > cameraX + renderWidth + 16.0f);
}

}  // namespace LightsOut
