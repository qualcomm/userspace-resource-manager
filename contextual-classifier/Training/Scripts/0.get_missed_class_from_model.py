import floret
from collections import defaultdict

"""
    ## Intruction to run
    python Scripts/0.get_missed_class_from_model.py 

"""

TEST_FILE = 'supervised/learn_browsers/data.test'
MODEL_FILE = 'supervised/learn_browsers/model_ov15_ep150_di60_no_ngrams.bin'

# Load model
print(f"Loading model: {MODEL_FILE}")
model = floret.load_model(MODEL_FILE)
print("✓ Model loaded successfully\n")

# Initialize
true_labels = []
pred_labels = []
misclassifications = []

print("=== Processing Test File ===")

# Process test file
with open(TEST_FILE, 'r', encoding='utf-8') as f:
    for line_num, line in enumerate(f, 1):
        if not line.strip():
            continue
        
        parts = line.strip().split(' ')
        
        # Extract true label
        true_label = parts[0].replace('__label__', '')
        true_labels.append(true_label)
        
        # Extract text
        text = ' '.join(parts[1:])
        
        # Predict
        pred = model.predict(text, k=1)
        pred_label = pred[0][0].replace('__label__', '')
        pred_prob = pred[1][0]
        pred_labels.append(pred_label)
        
        # Track misclassifications
        if true_label != pred_label:
            misclassifications.append({
                'line': line_num,
                'true': true_label,
                'pred': pred_label,
                'prob': pred_prob,
                'text': text[:100]  # First 100 chars
            })

# Calculate metrics
total = len(true_labels)
correct = sum(1 for t, p in zip(true_labels, pred_labels) if t == p)
incorrect = total - correct
accuracy = (correct / total * 100) if total > 0 else 0

# Print summary
print(f"\n{'='*60}")
print(f"=== Test Results ===")
print(f"{'='*60}")
print(f"Total samples: {total}")
print(f"Correct predictions: {correct}")
print(f"Incorrect predictions: {incorrect}")
print(f"Accuracy: {accuracy:.2f}%")

# Print misclassifications
if misclassifications:
    print(f"\n{'='*60}")
    print(f"=== Misclassifications ({len(misclassifications)} total) ===")
    print(f"{'='*60}")
    
    for i, misc in enumerate(misclassifications[:20], 1):  # Show first 20
        print(f"\n{i}. Line {misc['line']}")
        print(f"   True: {misc['true']:<10} | Predicted: {misc['pred']:<10} | Confidence: {misc['prob']:.4f}")
        print(f"   Text: {misc['text']}")
    
    if len(misclassifications) > 20:
        print(f"\n... and {len(misclassifications) - 20} more misclassifications")

# Per-class accuracy
print(f"\n{'='*60}")
print(f"=== Per-Class Accuracy ===")
print(f"{'='*60}")

class_stats = defaultdict(lambda: {'correct': 0, 'total': 0})

for true, pred in zip(true_labels, pred_labels):
    class_stats[true]['total'] += 1
    if true == pred:
        class_stats[true]['correct'] += 1

print(f"{'Class':<15} {'Correct':<10} {'Total':<10} {'Accuracy':<10}")
print("-" * 45)

for cls in sorted(class_stats.keys()):
    stats = class_stats[cls]
    acc = (stats['correct'] / stats['total'] * 100) if stats['total'] > 0 else 0
    print(f"{cls:<15} {stats['correct']:<10} {stats['total']:<10} {acc:.2f}%")
