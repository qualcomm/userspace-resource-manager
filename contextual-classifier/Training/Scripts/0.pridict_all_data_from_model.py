#!/usr/bin/env python3
import floret
import pandas as pd
import re
from sklearn.metrics import classification_report, accuracy_score

"""
    ## Intruction to run
    python Scripts/0.pridict_all_data_from_model.py 

"""

CORPUS_FILE = 'Corpus/corpus_1006_dataset.csv'
MODEL_FILE = 'supervised/learn_browsers/model_ov15_ep150_di60_no_ngrams.bin'

# ========== COPY THE EXACT clean_text() FROM TRAINING SCRIPT ==========
REMOVE_KEYWORDS = {
    'unconfined', 'user.slice', 'user-n.slice', 'user@n.service', 
    'app.slice', 'app-org.gnome.terminal.slice', 'vte-spawn-n.scope'
}

def clean_text(line: str) -> str:
    """EXACT copy from training script"""
    if not isinstance(line, str):
        return ""
    line = line.lower()
    line = re.sub(r'[\[\]\(\)\{\}]', ' ', line)
    tokens = line.replace(',', ' ').split()
    clean_tokens = []
    seen = set()
    for token in tokens:
        t = token.strip()
        if t in REMOVE_KEYWORDS or \
           re.match(r'^user-\d+\.slice$', t) or \
           re.match(r'^user@\d+\.service$', t) or \
           re.match(r'^vte-spawn-.*\.scope$', t) or \
           re.match(r'^\d+(\.\d+)?$', t):
            continue
        t = re.sub(r'0x[a-f0-9]+', '<hex>', t)
        t = re.sub(r'\d{4,}', '<num>', t)


        if t and len(t) > 1:
            clean_tokens.append(t)

    return " ".join(clean_tokens)

# ========== LOAD MODEL ==========
print("Loading model...")
model = floret.load_model(MODEL_FILE)
print(f"✓ Model loaded")
print(f"  Labels: {model.get_labels()}")

# ========== LOAD CORPUS ==========
print("\nLoading corpus...")
df = pd.read_csv(CORPUS_FILE, on_bad_lines='skip', engine='python', header=None)
print(f"✓ Loaded {len(df)} rows, {len(df.columns)} columns")

# ========== PREPARE DATA (SAME AS TRAINING) ==========
print("\nPreparing data...")
feature_cols = df.columns[1:10]
label_col = df.columns[-1]

column_weights = {
    1: 1,  # attr (normal weight)
    2: 1,  # cgroup (normal weight)
    3: 5,  # cmdline (2x weight - important!)
    4: 5,  # comm (2x weight - very important!)
    5: 1,  # maps (2x weight - important!)
    6: 1,  # fds (2x weight - important!)
    7: 1,  # environ (normal weight)
    8: 5,  # exe (2x weight - important!)
    9: 1   # logs (normal weight)
}
    
# ✓ BUILD WEIGHTED TEXT
df['cleaned_text'] = df[feature_cols].apply(
    lambda r: " ".join([
        " ".join([str(r[col])] * column_weights.get(col, 1))  # Repeat based on weight
        for col in feature_cols 
        if pd.notna(r[col])
    ]), 
    axis=1
).apply(clean_text)

df['label'] = df[label_col].astype(str).str.strip().str.replace(' ', '_')

# Remove invalid rows (same as training)
df = df[df['cleaned_text'] != ''].copy()

print(f"✓ Valid samples: {len(df)}")
print(f"\nClass distribution:")
print(df['label'].value_counts())

# ========== EVALUATE ==========
print(f"\n{'='*60}")
print("Evaluating model...")
print(f"{'='*60}\n")

true_labels = []
pred_labels = []
misclassifications = []

for idx, row in df.iterrows():
    true_label = row['label']
    text = row['cleaned_text']  # Use cleaned_text!

    ##print(f"concatenated text")
    ##print(text)
    
    # Predict
    pred = model.predict(text, k=1)
    pred_label = pred[0][0].replace('__label__', '')
    pred_prob = pred[1][0]
    
    true_labels.append(true_label)
    pred_labels.append(pred_label)
    
    # Track misclassifications
    if true_label != pred_label:
        misclassifications.append({
            'index': idx,
            'true': true_label,
            'pred': pred_label,
            'prob': pred_prob,
            'text': text[:100]
        })

# ========== RESULTS ==========
accuracy = accuracy_score(true_labels, pred_labels)
total = len(true_labels)
correct = sum(1 for t, p in zip(true_labels, pred_labels) if t == p)

print(f"{'='*60}")
print("=== Results ===")
print(f"{'='*60}")
print(f"Total samples: {total}")
print(f"Correct: {correct}")
print(f"Incorrect: {total - correct}")
print(f"Accuracy: {accuracy * 100:.2f}%")

print(f"\n{'='*60}")
print("=== Classification Report ===")
print(f"{'='*60}")
print(classification_report(true_labels, pred_labels, zero_division=0))

# Show misclassifications
if misclassifications:
    print(f"\n{'='*60}")
    print(f"=== Top 20 Misclassifications ===")
    print(f"{'='*60}")
    
    misclassifications.sort(key=lambda x: x['prob'], reverse=True)
    
    for i, misc in enumerate(misclassifications[:25], 1):
        print(f"\n{i}. Index {misc['index']}")
        print(f"   True: {misc['true']:<10} | Pred: {misc['pred']:<10} | Confidence: {misc['prob']:.4f}")
        print(f"   Text: {misc['text']}")

print(f"\n{'='*60}")
print("Evaluation complete!")
print(f"{'='*60}")