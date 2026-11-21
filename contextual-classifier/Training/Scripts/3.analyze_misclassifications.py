import fasttext
import pandas as pd
from sklearn.metrics import confusion_matrix, classification_report
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np
import argparse
import os
import sys

# Configuration
INPUT_FILE = 'supervised/corpus.csv'
TRAIN_FILE_PATH = 'supervised/data.train'
VALID_DATA_PATH = 'supervised/data.valid'
DEFAULT_MODEL_PATH = 'supervised/Models/model_unlimited.bin'

# --- Data Cleaning (Copied from other scripts for consistency) ---
REMOVE_KEYWORDS = {
    'unconfined', 'user.slice', 'user-n.slice', 'user@n.service', 
    'app.slice', 'app-org.gnome.terminal.slice', 'vte-spawn-n.scope'
}

def clean_text(line: str) -> str:
    if not isinstance(line, str): return ""
    line = line.lower()
    tokens = line.replace(',', ' ').split()
    clean_tokens = []
    seen = set()
    for token in tokens:
        t = token.strip()
        if t in REMOVE_KEYWORDS or re.match(r'^user-\d+\.slice$', t) or re.match(r'^user@\d+\.service$', t) or re.match(r'^vte-spawn-.*\.scope$', t) or re.match(r'^\d+(\.\d+)?$', t):
            continue
        t = re.sub(r'0x[a-f0-9]+', '<hex>', t)
        t = re.sub(r'\d{4,}', '<num>', t) 
        if t and t not in seen:
            seen.add(t)
            clean_tokens.append(t)
    return " ".join(clean_tokens)

def prepare_data_files():
    """
    Generates data.train and data.valid if they don't exist.
    """
    if os.path.exists(VALID_DATA_PATH) and os.path.exists(TRAIN_FILE_PATH):
        print("Found existing data.train and data.valid files. Skipping generation.")
        return

    print("--- data.train/valid not found. Generating from corpus.csv... ---")
    if not os.path.exists(INPUT_FILE):
        print(f"Error: {INPUT_FILE} not found. Cannot generate data files.")
        sys.exit(1) # Exit because the script cannot proceed

    df = pd.read_csv(INPUT_FILE, on_bad_lines='skip', engine='python', header=None)
    feature_cols, label_col = df.columns[1:-1], df.columns[-1]
    df['cleaned_text'] = df[feature_cols].apply(lambda r: " ".join([str(c) for c in r if pd.notna(c)]), axis=1).apply(clean_text)
    df['label'] = df[label_col].astype(str).str.strip().str.replace(' ', '_')
    df = df[df['cleaned_text'] != ''].copy()
    df['ft_line'] = '__label__' + df['label'] + ' ' + df['cleaned_text']

    # Create a standard, non-oversampled, stratified split
    train_df, valid_df = train_test_split(df, test_size=0.2, random_state=42, stratify=df['label'])

    train_df['ft_line'].to_csv(TRAIN_FILE_PATH, index=False, header=False)
    valid_df['ft_line'].to_csv(VALID_DATA_PATH, index=False, header=False)
    print(f"Successfully generated {TRAIN_FILE_PATH} and {VALID_DATA_PATH}")


def load_validation_data(path):
    texts = []
    labels = []
    with open(path, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line: continue
            
            # FastText format: __label__class text...
            # Split on first space
            parts = line.split(' ', 1)
            if len(parts) < 2:
                continue
            
            label_part = parts[0]
            text_part = parts[1]
            
            if label_part.startswith('__label__'):
                label = label_part.replace('__label__', '')
                labels.append(label)
                texts.append(text_part)
                
    return texts, labels

def main():
    # First, ensure data files exist before parsing args
    prepare_data_files()

    parser = argparse.ArgumentParser(description="Evaluate FastText model misclassifications.")
    parser.add_argument('model_path', nargs='?', default=DEFAULT_MODEL_PATH, help="Path to the FastText model .bin file")
    parser.add_argument('--data', default=VALID_DATA_PATH, help="Path to evaluation data (.valid or .txt)")
    args = parser.parse_args()

    model_path = args.model_path
    data_path = args.data

    if not os.path.exists(model_path):
        print(f"Error: Model file '{model_path}' not found.")
        return
    
    if not os.path.exists(data_path):
        print(f"Error: Data file '{data_path}' not found.")
        return

    # Derive output report path
    model_name = os.path.splitext(os.path.basename(model_path))[0]
    data_name = os.path.splitext(os.path.basename(data_path))[0]
    report_path = f"supervised/{model_name}_on_{data_name}_report.txt"

    print(f"Analyzing model: {model_path}")
    print(f"Using data: {data_path}")
    print(f"Saving report to: {report_path}")

    # Redirect stdout to File ONLY
    original_stdout = sys.stdout
    sys.stdout = open(report_path, "w")

    try:
        print(f"--- Model Evaluation Report: {model_name} on {data_name} ---\n")
        print(f"Loading model from {model_path}...")
        try:
            model = fasttext.load_model(model_path)
        except ValueError as e:
            print(f"Error loading model: {e}")
            return

        print(f"Loading validation data from {data_path}...")
        texts, true_labels = load_validation_data(data_path)
        print(f"Loaded {len(texts)} samples.")

        pred_labels = []
        results = []

        print("\n--- Predictions ---")
        print(f"{'Actual':<15} | {'Predicted':<15} | {'Text'}")
        print("-" * 100)

        for text, actual in zip(texts, true_labels):
            # Predict
            # k=1 returns top 1 label
            labels, probs = model.predict(text, k=1)
            predicted = labels[0].replace('__label__', '')
            
            pred_labels.append(predicted)
            
            # Mark misclassifications
            status = "CORRECT" if actual == predicted else "MISMATCH"
            results.append({
                'text': text,
                'actual': actual,
                'predicted': predicted,
                'status': status
            })
            
            print(f"{actual:<15} | {predicted:<15} | {text[:60]}...")

        # Create DataFrame for analysis
        df = pd.DataFrame(results)
        
        # Filter for misclassifications
        misclassified = df[df['status'] == 'MISMATCH']
        
        print("\n\n--- Misclassification Summary ---")
        print(f"Total Samples: {len(df)}")
        print(f"Correct: {len(df) - len(misclassified)}")
        print(f"Misclassified: {len(misclassified)}")
        print(f"Accuracy: {(len(df) - len(misclassified)) / len(df):.4f}")

        if not misclassified.empty:
            print("\n--- Detailed Misclassifications ---")
            for idx, row in misclassified.iterrows():
                print(f"Text: {row['text']}")
                print(f"  Actual: {row['actual']}")
                print(f"  Predicted: {row['predicted']}")
                print("-" * 30)
        else:
            print("\nGreat! No misclassifications found.")

        # Metrics
        unique_labels = sorted(list(set(true_labels + pred_labels)))
        
        print("\n--- Classification Report ---")
        print(classification_report(true_labels, pred_labels, labels=unique_labels, zero_division=0))

        print("\n--- Confusion Matrix ---")
        cm = confusion_matrix(true_labels, pred_labels, labels=unique_labels)
        
        # Print readable CM
        print(f"{'':<10}", end="")
        for label in unique_labels:
            print(f"{label:<10}", end="")
        print()
        
        for i, row in enumerate(cm):
            print(f"{unique_labels[i]:<10}", end="")
            for val in row:
                print(f"{val:<10}", end="")
            print()
            
        print(f"\nReport saved to: {report_path}")

    finally:
        sys.stdout.close()
        sys.stdout = original_stdout
        print("Done.")

if __name__ == "__main__":
    main()
