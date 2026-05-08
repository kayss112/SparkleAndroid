#pragma once

#include "core/Path.h"
#include "core/task/TaskFuture.h"

namespace sparkle
{
class Scene;
class UiManager;

class SceneManager
{
public:
    [[nodiscard]] static std::shared_ptr<TaskFuture<void>> LoadScene(Scene *scene, const Path &asset_path,
                                                                     bool need_default_sky, bool need_default_lighting,
                                                                     const std::string &scene_name = "TestScene");
    


    static void RemoveLastDebugSphere(Scene *scene);

    static void GenerateRandomSpheres(Scene &scene, unsigned count);

    [[nodiscard]] static std::shared_ptr<TaskFuture<void>> AddDefaultSky(Scene *scene);

    static void AddDefaultDirectionalLight(Scene *scene);

    static void DrawUi(Scene *scene, bool need_default_sky, bool need_default_lighting);

//wen
    static void AddSceneToList(const std::string &name, std::function<std::shared_ptr<TaskFuture<void>>(Scene *)> fn);

    static void InitSceneList();

private:
    static std::unordered_map<std::string, std::function<std::shared_ptr<TaskFuture<void>>(Scene*)>> SceneList;
};
} // namespace sparkle
