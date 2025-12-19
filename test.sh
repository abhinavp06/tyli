#!/bin/bash

# Define Project Root
VAULT_NAME="Master_Bit_Crusher_Vault"
PHASE_DIR="$VAULT_NAME/07_Phase_7_Quality_Benchmarking"

echo "?? Initializing Phase 7: Perceptual Quality Comparer..."

mkdir -p "$PHASE_DIR"

touch "$PHASE_DIR/01_Time_Alignment_and_Cross_Correlation.md"
touch "$PHASE_DIR/02_The_Delta_Signal_Analysis.md"
touch "$PHASE_DIR/03_Spectral_Flatness_and_THD.md"
touch "$PHASE_DIR/04_Psychoacoustic_Masking_Models.md"
touch "$PHASE_DIR/05_PEAQ_and_Objective_Scoring.md"

echo "? Quality Benchmarking Phase added."