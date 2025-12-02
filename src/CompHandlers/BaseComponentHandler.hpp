#pragma once
class BaseComponentHandler {
public:
    virtual ~BaseComponentHandler() = default;
    virtual bool initialize() = 0;
    virtual bool shutdown() = 0;
};