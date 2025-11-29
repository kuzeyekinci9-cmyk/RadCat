#define COMPONENT template<typename... Components> class BaseDevice; class EmptyDevice;

class EmptyDevice;
class BaseComponent {
public:
    BaseComponent(EmptyDevice* parentDevice) { parent = parentDevice; }
    virtual ~BaseComponent() = default;
    virtual void update(){}
    virtual void initialize(){}
    EmptyDevice* parent;
};