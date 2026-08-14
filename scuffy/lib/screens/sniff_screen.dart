import 'dart:async';

import 'package:flutter/material.dart';

import '../models/sniff_session.dart';
import '../services/ble_service.dart';

/// Pantalla de volcado de tramas CAN (protocolo v2 — design sniff).
///
/// Activa/desactiva el sniff del firmware (`sniff:on` / `sniff:off`) y
/// muestra en vivo las líneas `sniff:...` y la alerta `can:no_frames`.
/// Todo el estado de la sesión vive en [SniffSession] (modelo puro,
/// testeado a nivel unitario); esta pantalla solo lo renderiza.
class SniffScreen extends StatefulWidget {
  final BleService bluetooth;
  final Color accentColor;

  const SniffScreen({
    super.key,
    required this.bluetooth,
    required this.accentColor,
  });

  @override
  State<SniffScreen> createState() => _SniffScreenState();
}

class _SniffScreenState extends State<SniffScreen> {
  SniffSession session = SniffSession.initial();

  StreamSubscription<String>? _sniffSubscription;
  StreamSubscription<bool?>? _canStatusSubscription;
  StreamSubscription<int>? _protocolSubscription;

  @override
  void initState() {
    super.initState();

    _sniffSubscription = widget.bluetooth.sniffStream.listen((message) {
      if (!mounted) return;
      setState(() {
        session = session.handleMessage(message);
      });
    });

    _canStatusSubscription =
        widget.bluetooth.canStatusStream.listen((online) {
      if (!mounted) return;
      setState(() {
        session = session.copyWith(canOnline: online);
      });
    });

    _protocolSubscription =
        widget.bluetooth.protocolVersionStream.listen((version) {
      if (!mounted) return;
      setState(() {
        session = session.copyWith(protocolVersion: version);
      });
    });

    // Estado fresco de CAN/proto al abrir la pantalla.
    widget.bluetooth.requestState();
  }

  @override
  void dispose() {
    _sniffSubscription?.cancel();
    _canStatusSubscription?.cancel();
    _protocolSubscription?.cancel();
    super.dispose();
  }

  Future<void> _toggleSniff() async {
    final enabled = !session.enabled;
    setState(() {
      session = session.copyWith(enabled: enabled);
    });
    await widget.bluetooth.sendSniff(enabled);
  }

  void _clearFrames() {
    setState(() {
      session = session.clearFrames();
    });
  }

  // ── Chips de estado ──────────────────────────────────

  Widget _buildCanChip() {
    final online = session.canOnline;

    final (color, label) = switch (online) {
      true => (widget.accentColor, "CAN: Conectado"),
      false => (Colors.red, "CAN: Sin datos"),
      null => (Colors.grey, "CAN: —"),
    };

    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 10),
      decoration: BoxDecoration(
        color: color.withValues(alpha: 0.12),
        borderRadius: BorderRadius.circular(12),
        border: Border.all(color: color.withValues(alpha: 0.4), width: 1),
      ),
      child: Row(
        children: [
          Icon(Icons.circle, size: 10, color: color),
          const SizedBox(width: 8),
          Text(
            label,
            style: TextStyle(
              fontSize: 13,
              color: color,
              fontWeight: FontWeight.w600,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildProtocolChip() {
    final version = session.protocolVersion;

    final (color, label) = switch (version) {
      == 2 => (widget.accentColor, "FW v2"),
      _ => (Colors.orange, "FW: actualizá el knob"),
    };

    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 10),
      decoration: BoxDecoration(
        color: color.withValues(alpha: 0.12),
        borderRadius: BorderRadius.circular(12),
        border: Border.all(color: color.withValues(alpha: 0.4), width: 1),
      ),
      child: Row(
        children: [
          Icon(Icons.memory, size: 14, color: color),
          const SizedBox(width: 8),
          Text(
            label,
            style: TextStyle(
              fontSize: 13,
              color: color,
              fontWeight: FontWeight.w600,
            ),
          ),
        ],
      ),
    );
  }

  // ── Lista de frames ─────────────────────────────────

  Widget _buildFrameList() {
    if (session.frames.isEmpty) {
      return const Center(
        child: Text(
          "Sin tramas capturadas.\nActivá SNIFF para ver el bus CAN.",
          textAlign: TextAlign.center,
          style: TextStyle(color: Colors.white54),
        ),
      );
    }

    return ListView.builder(
      padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 8),
      itemCount: session.frames.length,
      itemBuilder: (context, index) {
        final frame = session.frames[index];
        return Padding(
          padding: const EdgeInsets.only(bottom: 6),
          child: Container(
            padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 10),
            decoration: BoxDecoration(
              color: Colors.white.withValues(alpha: 0.05),
              borderRadius: BorderRadius.circular(8),
            ),
            child: Text(
              frame,
              style: const TextStyle(
                fontFamily: 'monospace',
                fontSize: 13,
                color: Colors.white70,
              ),
            ),
          ),
        );
      },
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: const Color(0xFF0B0B0B),
      appBar: AppBar(
        title: const Text("Sniff CAN"),
        centerTitle: true,
      ),
      body: SafeArea(
        child: Column(
          children: [
            Padding(
              padding: const EdgeInsets.all(20),
              child: Row(
                children: [
                  _buildCanChip(),
                  const SizedBox(width: 10),
                  _buildProtocolChip(),
                  const Spacer(),
                ],
              ),
            ),

            if (session.busSilent)
              Padding(
                padding: const EdgeInsets.symmetric(horizontal: 20),
                child: Container(
                  width: double.infinity,
                  padding: const EdgeInsets.all(12),
                  decoration: BoxDecoration(
                    color: Colors.orange.withValues(alpha: 0.12),
                    borderRadius: BorderRadius.circular(10),
                    border: Border.all(
                      color: Colors.orange.withValues(alpha: 0.4),
                      width: 1,
                    ),
                  ),
                  child: const Row(
                    children: [
                      Icon(Icons.warning_amber, size: 16, color: Colors.orange),
                      SizedBox(width: 8),
                      Expanded(
                        child: Text(
                          "Bus silencioso: sin tramas CAN (¿desconectado?)",
                          style: TextStyle(fontSize: 13, color: Colors.orange),
                        ),
                      ),
                    ],
                  ),
                ),
              ),

            Expanded(child: _buildFrameList()),

            Padding(
              padding: const EdgeInsets.all(20),
              child: Row(
                children: [
                  Expanded(
                    child: SizedBox(
                      height: 50,
                      child: ElevatedButton.icon(
                        onPressed: _toggleSniff,
                        icon: Icon(
                          session.enabled
                              ? Icons.pause_circle
                              : Icons.play_circle,
                        ),
                        label: Text(
                          session.enabled ? "SNIFF: ON" : "SNIFF: OFF",
                          style: const TextStyle(fontWeight: FontWeight.w600),
                        ),
                        style: ButtonStyle(
                          backgroundColor:
                              WidgetStateProperty.all(widget.accentColor),
                          shape: WidgetStateProperty.all(
                            RoundedRectangleBorder(
                              borderRadius: BorderRadius.circular(14),
                            ),
                          ),
                        ),
                      ),
                    ),
                  ),
                  const SizedBox(width: 12),
                  IconButton(
                    onPressed:
                        session.frames.isEmpty ? null : _clearFrames,
                    icon: const Icon(Icons.delete_sweep),
                    tooltip: "Limpiar tramas",
                    color: Colors.white70,
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }
}
