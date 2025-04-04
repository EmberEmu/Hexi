#include <hexi.h>
#include <algorithm>
#include <iostream>
#include <vector>

struct Component {
    int weight;
    int combobulating_factor;

    bool operator==(const Component& rhs) const {
        return weight == rhs.weight
            && combobulating_factor == rhs.combobulating_factor;
    }
};

class Widget {
    std::vector<Component> components;
    
public:
    void add_component(Component&& c) {
        components.emplace_back(std::move(c));
    }

    void serialise(auto& stream) {
        stream & hexi::prefixed(components);
    }

    bool operator==(const Widget& rhs) const {
        return components == rhs.components;
    }
};

int main() {
    // note: you can use prefixed or prefixed_varint
    // prefixed: uses a 32-bit unsigned integer to store container size
    // prefixed_varint: compresses container size into a variable number of bytes (more compact but slower)
    std::vector<char> buffer;
    hexi::buffer_adaptor ba(buffer);
    hexi::binary_stream stream(ba);

    { // trivial types just work
        std::vector input { 1, 2, 3, 4, 5 };
        stream << hexi::prefixed(input);

        std::vector<int> output;
        stream >> hexi::prefixed(output);
        std::cout << std::boolalpha << (input == output) << '\n';
    }

    { // more complicated types
        std::vector<Widget> input;

        // make some widgets
        for(auto i = 0; i < 5; ++i) {
            Widget w;
            w.add_component({ 100, i });
            input.emplace_back(std::move(w));
        }

        stream << hexi::prefixed(input);

        // get them back out
        std::vector<Widget> output;
        stream >> hexi::prefixed(output);
        std::cout << std::boolalpha << (input == output);
    }
}