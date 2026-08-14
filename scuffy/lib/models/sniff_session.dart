/// Immutable view-state for the sniff screen. Fed by the BLE notification
/// stream and mutated through pure `handleMessage`/`toggle` so the screen
/// stays a thin renderer (unit-tested without widgets or BLE).
class SniffSession {
  /// Newest frames kept in memory (drop oldest beyond this cap).
  static const int maxFrames = 200;

  final bool enabled;
  final List<String> frames;
  final bool? canOnline; // null = unknown yet
  final int? protocolVersion;
  final bool busSilent; // a can:no_frames watchdog line was received

  const SniffSession({
    required this.enabled,
    required this.frames,
    required this.canOnline,
    required this.protocolVersion,
    required this.busSilent,
  });

  factory SniffSession.initial() => const SniffSession(
        enabled: false,
        frames: [],
        canOnline: null,
        protocolVersion: null,
        busSilent: false,
      );

  SniffSession copyWith({
    bool? enabled,
    List<String>? frames,
    bool? canOnline,
    int? protocolVersion,
    bool? busSilent,
  }) {
    return SniffSession(
      enabled: enabled ?? this.enabled,
      frames: frames ?? this.frames,
      canOnline: canOnline ?? this.canOnline,
      protocolVersion: protocolVersion ?? this.protocolVersion,
      busSilent: busSilent ?? this.busSilent,
    );
  }

  /// Applies one BLE notification line to the session state.
  SniffSession handleMessage(String message) {
    if (message.startsWith('sniff:')) {
      final frames = [...this.frames, message];
      if (frames.length > maxFrames) {
        frames.removeRange(0, frames.length - maxFrames);
      }
      return copyWith(frames: frames, busSilent: false);
    }

    if (message == 'can:no_frames') {
      return copyWith(busSilent: true);
    }

    if (message == 'can:ok') {
      return copyWith(canOnline: true, busSilent: false);
    }

    if (message == 'can:offline') {
      return copyWith(canOnline: false);
    }

    if (message.startsWith('proto:')) {
      final version = int.tryParse(message.substring(6));
      if (version != null) {
        return copyWith(protocolVersion: version);
      }
    }

    return this;
  }

  /// Flips the sniff capture toggle.
  SniffSession toggle() => copyWith(enabled: !enabled);

  /// Clears the captured frames and the silent-bus warning.
  SniffSession clearFrames() =>
      copyWith(frames: const [], busSilent: false);
}
