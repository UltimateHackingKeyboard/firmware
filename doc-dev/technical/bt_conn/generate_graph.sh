#!/bin/bash

# ============================================================================
# Bluetooth Connection Sequence Graph Generator
# ============================================================================
#
# This script generates an SVG visualization of the Bluetooth connection 
# initiation sequence from the connection_sequence.dot file and opens it
# in an image viewer.
#
# HOW THIS GRAPH WAS CREATED:
# ---------------------------
# The graph was created by analyzing the Bluetooth connection code in the 
# device/src/bt_* files, particularly bt_conn.c, with the assistance of
# Claude 3.7 Sonnet AI. The analysis focused on:
#
# 1. Connection establishment flow:
#    - How connections are initially established
#    - How known vs unknown connections are handled
#    - Service discovery for unknown connections
#
# 2. Pairing and authentication:
#    - How pairing is initiated (by remote device)
#    - Authentication callbacks and user interaction
#    - Different paths for OOB pairing, known BLE HID, and new BLE HID connections
#
# 3. Connection type handling:
#    - NUS client/server connection establishment
#    - BLE HID connection establishment
#    - Message exchange required to complete NUS connections
#
# 4. State transitions:
#    - When connections move from Connected to Ready state
#    - How peer assignments are handled
#
# The graph uses dotted nodes to represent waiting states (where the system
# waits for external events) and solid nodes for active processing steps.
#
# To update this graph:
# 1. Analyze the connection flow in the code
# 2. Identify key functions and state transitions
# 3. Map out the temporal sequence of events
# 4. Pay special attention to branching logic and waiting states
# 5. Update the .dot file with the new understanding
# 6. Run this script to generate and view the updated graph
#
# ============================================================================

# Generate SVG and open with gpicview
dot -Tsvg connection_sequence.dot -o connection_sequence.svg
gpicview connection_sequence.svg 