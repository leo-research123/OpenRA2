#pragma once
namespace game {
struct PresentationContext;
void build_planning_underlay(PresentationContext&) noexcept;
void build_planning_overlay(PresentationContext&) noexcept;
void build_planning_feedback(PresentationContext&) noexcept;
}
