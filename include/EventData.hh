#ifndef EVENTDATA_HH
#define EVENTDATA_HH

class EventData {
 public:
  static EventData& Instance();

  void Reset();
  void AddEnergyDeposit(double edep);
  void AddScintillationPhotons(int count);

  double GetEnergyDeposit() const;
  int GetScintillationPhotons() const;

 private:
  EventData() = default;

  double fEnergyDeposit = 0.0;
  int fScintillationPhotons = 0;
};

#endif
