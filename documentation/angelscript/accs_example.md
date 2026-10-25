```
class Health : CE::ACCS::IComponent {
    int value = 100;
};

class HealthSystem : CE::ACCS::ISystem {
    void Update(float dt, CE::ACCS::FrameUpdateType phase) {
        array<CE::ACCS::ActorHandle>@ actors = CE::ACCS::GetActors();
        for (uint i = 0; i < actors.length(); ++i) {
            array<CE::ACCS::ComponentID>@ components = CE::ACCS::GetComponents(actors[i]);
            for (uint j = 0; j < components.length(); ++j) {
                Health@ health = cast<Health>(CE::ACCS::GetComponent(components[j]));
                if (health !is null) {
                    health.value += int(dt);
                }
            }
        }
    }
};
void main() {
    CE::ACCS::ActorHandle actor = CE::ACCS::CreateActor();
    Health@ health = Health();
    CE::ACCS::AddComponent(actor, health);
    CE::ACCS::RegisterSystem(CE::ACCS::FrameUpdateType::Logic, HealthSystem());
}
```