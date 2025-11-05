#ifndef TRAIN_HPP
#define TRAIN_HPP

class Train {
private:
    int capacity;
    int startIndex;
    int endIndex;
    int direction;

public:
    Train(int capacity, int startIndex, int endIndex, int direction);
    int getId() const;
    int getCapacity() const;
    int getStartIndex() const;
    int getEndIndex() const;
    int getDirection() const;
    ~Train();
};

#endif