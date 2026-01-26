import faelight.log;
import faelight.signal;

int main() {

    FL::Log::Config::setLogLevel(FL::Log::eLogLevel::DEBUG);
    FL::Log::info("Hello! Can I have your name? <3");

    FL::Signal<int, int> signal;
    signal.connect([&](auto num, auto num2) {
        FL::Log::err("One shot {} and {}", num, num2);
    }, true);
    signal.connect([&](auto num, auto num2) {
       FL::Log::err("Continues {} and {}", num, num2);
    });
    signal.trigger(43, 54);
    signal(54,65);

    return 0;
}