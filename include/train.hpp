#ifndef TRAIN_HPP
#define TRAIN_HPP

class Train {
  private:
  int capacity;
  int startIndex;
  int endIndex;
  int direction;

  constexpr static int max_capacity = 1000;

  public:
  Train(int capacity, int startIndex, int endIndex, int direction, int passenger_count = 0);
  int getId() const;
  int getCapacity() const;
  int getStartIndex() const;
  int getEndIndex() const;
  int getDirection() const;
  void setDirection(int newDirection);
  ~Train();

  int passenger_count;
};

#endif