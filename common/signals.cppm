module;

#include <functional>

export module faelight.signal;

export namespace FL {

    template <typename ...Args>struct sCallback {
        std::function<void(Args...)> raw_func_ptr = {};
        bool                      one_shot = false;
    };

    template <typename ...Args> class Signal{
    public:

        void connect(std::function<void(Args...)> func_ptr, const bool one_shot = false) {
            callbacks.push_back({func_ptr, one_shot});
        }

        void trigger(Args... args) {

            for (auto it = callbacks.begin(); it != callbacks.end();) {

                it->raw_func_ptr(std::forward<Args>(args)...);
                if (it->one_shot) {
                    it = callbacks.erase(it);
                }else {
                    ++it;
                }
            }
        }

        void operator()(Args... args) {
            trigger(std::forward<Args>(args)...);
        }

    private:

        std::vector<sCallback<Args...>> callbacks = {};
    };

}
