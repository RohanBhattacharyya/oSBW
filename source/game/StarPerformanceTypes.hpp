#pragma once

#include "StarVector.hpp"

namespace Star {

// Level of Detail for entity rendering and updates
// Used to reduce computational load for distant/off-screen entities
enum class EntityLOD : uint8_t {
  Full = 0,       // Full detail - animations at full rate, full rendering
  Reduced = 1,    // Reduced detail - half animation rate
  Minimal = 2,    // Minimal detail - static sprites, minimal updates
  Culled = 3      // Culled - no rendering, minimal updates
};

// Performance configuration settings
struct PerformanceSettings {
  // LOD settings
  bool entityLODEnabled = true;
  float lodReducedDistance = 200.0f;   // Screen distance for reduced LOD
  float lodMinimalDistance = 400.0f;   // Screen distance for minimal LOD
  float lodCulledDistance = 800.0f;    // Screen distance for culled LOD
  
  // Particle limits
  size_t maxVisibleParticles = 5000;
  
  // Adaptive script rates
  bool adaptiveScriptRatesEnabled = true;
  int idleScriptDelta = 10;            // Script delta when entity is idle (6 Hz)
  int activeScriptDelta = 1;           // Script delta when entity is active (60 Hz)
  
  // Light culling
  float lightCullingRadius = 100.0f;   // Lights beyond this are culled
  
  // Entity updates
  size_t maxEntitiesPerUpdate = 0;     // 0 = unlimited
};

// Calculate LOD based on distance from camera
inline EntityLOD calculateEntityLOD(Vec2F entityPos, Vec2F cameraCenter, float cameraZoom, PerformanceSettings const& settings) {
  if (!settings.entityLODEnabled)
    return EntityLOD::Full;
    
  float dist = (entityPos - cameraCenter).magnitude();
  float screenDist = dist * cameraZoom;
  
  if (screenDist > settings.lodCulledDistance)
    return EntityLOD::Culled;
  if (screenDist > settings.lodMinimalDistance)
    return EntityLOD::Minimal;
  if (screenDist > settings.lodReducedDistance)
    return EntityLOD::Reduced;
  return EntityLOD::Full;
}

// Get animation delta time multiplier for LOD level
inline float lodAnimationMultiplier(EntityLOD lod) {
  switch (lod) {
    case EntityLOD::Reduced:
      return 0.5f;   // Half animation speed
    case EntityLOD::Minimal:
    case EntityLOD::Culled:
      return 0.0f;   // Frozen animation
    default:
      return 1.0f;   // Full speed
  }
}

// Check if entity should be rendered at this LOD
inline bool shouldRenderAtLOD(EntityLOD lod) {
  return lod != EntityLOD::Culled;
}

// Check if entity should run scripts at this LOD
inline bool shouldUpdateScriptsAtLOD(EntityLOD lod) {
  return lod == EntityLOD::Full || lod == EntityLOD::Reduced;
}

}
