enum Gear { reverse, first, second, third, fourth, fifth, neutral }

extension GearExtension on Gear {
  String get name {
    switch (this) {
      case Gear.reverse:
        return "R";
      case Gear.first:
        return "1";
      case Gear.second:
        return "2";
      case Gear.third:
        return "3";
      case Gear.fourth:
        return "4";
      case Gear.fifth:
        return "5";
      case Gear.neutral:
        return "N";
    }
  }
}

class GearInfo {
  final Gear gear;

  bool calibrated;

  double w;
  double x;
  double y;
  double z;

  GearInfo({
    required this.gear,
    this.calibrated = false,
    this.w = 1,
    this.x = 0,
    this.y = 0,
    this.z = 0,
  });
}
