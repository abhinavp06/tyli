/*
* The Linerarity Scope is an X-Y oscilloscope.
* X-Axis -> Input sample value
* Y-Axis -> Output sample value
* Goal -> To verify if the dither actually blurs the steps or just adds noise on top
* It visualizes the "Transfer Function"
*		Ideal audio -> A straight diagonal line (y = x)
*		Quantized -> A staircase
* Success Metric: When dither is turned on, the sharp edges of the staircase should look fuzzy or smeared, effectively straightening the line on average.
* In a real plugin, this would be a GUI component. Here, it is being used as a data collector.
*/

#include <vector>

struct Point {
    float x; // clean input
    float y; // crushed output
};

class LinearityScope {
public:
    LinearityScope(int buffer_size = 512) {
        max_size = buffer_size;
        buffer.reserve(max_size);
    }

    void add_sample(float input_x, float output_y) {
        if (buffer.size() < max_size) {
            buffer.push_back({ input_x, output_y });
        }
    }

    bool is_full() const {
        return buffer.size() >= max_size;
    }

    // UI will call this to get data and draw the graph
    std::vector<Point> get_and_clear_buffer() {
        std::vector<Point> copy = buffer;
        buffer.clear();
        return copy;
    }

private:
    std::vector<Point> buffer;
    int max_size;
};