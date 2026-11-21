import pandas as pd
from collections import Counter, defaultdict
import math
import argparse

# Configuration
INPUT_FILE = 'supervised/corpus.csv'

def get_tokens(line):
    if not isinstance(line, str):
        return []
    # Basic tokenization
    return [t.strip() for t in line.lower().split(',') if t.strip()]

def analyze_corpus(input_path):
    print(f"Analyzing {input_path}...")
    try:
        df = pd.read_csv(input_path, on_bad_lines='skip', engine='python', header=None)
    except FileNotFoundError:
        print("File not found.")
        return

    if len(df.columns) < 2:
        print("CSV invalid.")
        return

    feature_cols = df.columns[1:-1]
    label_col = df.columns[-1]

    # Stats
    class_counts = Counter()
    word_counts = Counter()
    class_word_counts = defaultdict(Counter)
    
    total_docs = 0

    print("Processing corpus...")
    for _, row in df.iterrows():
        label = str(row[label_col]).strip()
        raw_text = ",".join([str(row[c]) for c in feature_cols if pd.notna(row[c])])
        tokens = get_tokens(raw_text)
        
        class_counts[label] += 1
        total_docs += 1
        
        seen_in_doc = set()
        for t in tokens:
            word_counts[t] += 1
            class_word_counts[label][t] += 1
            seen_in_doc.add(t)

    # 1. Top Distinctive Words per Class
    print("\n=== TOP DISTINCTIVE WORDS PER CLASS ===")
    print("(Words that strongly predict this class)")
    
    for label in class_counts:
        print(f"\n--- Class: {label} (Total: {class_counts[label]}) ---")
        scores = []
        for word, count in class_word_counts[label].items():
            if count < 5: continue # Ignore rare words
            
            # P(Class | Word) = Count(Word in Class) / Count(Word Total)
            prob_class_given_word = count / word_counts[word]
            
            # Penalize very rare global words (we want robust features)
            if word_counts[word] < 10:
                prob_class_given_word *= 0.5 
                
            scores.append((word, prob_class_given_word, count, word_counts[word]))
            
        # Sort by Probability desc, then Count desc
        scores.sort(key=lambda x: (x[1], x[2]), reverse=True)
        
        # Print top 15
        print(f"{'Word':<20} | {'Prob(Class|Word)':<10} | {'Count(In Class)':<10} | {'Total Count'}")
        for w, p, c, tc in scores[:15]:
            print(f"{w:<20} | {p:.2f}              | {c:<15} | {tc}")

    # 2. Top "Noise" Words (Common Stopwords)
    print("\n=== POTENTIAL NOISE WORDS (STOPWORDS) ===")
    print("(Words appearing frequently but evenly across classes)")
    
    noise_candidates = []
    num_classes = len(class_counts)
    
    for word, count in word_counts.items():
        if count < 50: continue # Only look at frequent words
        
        # Calculate Entropy of class distribution
        # High entropy = appeared in all classes evenly
        probs = []
        for label in class_counts:
            c = class_word_counts[label][word]
            if c > 0:
                probs.append(c / count)
        
        entropy = -sum(p * math.log(p) for p in probs)
        
        # Max entropy for N classes is log(N)
        max_entropy = math.log(num_classes) if num_classes > 1 else 1
        normalized_entropy = entropy / max_entropy if max_entropy > 0 else 0
        
        noise_candidates.append((word, normalized_entropy, count))
        
    # Sort by Entropy desc (most uniform)
    noise_candidates.sort(key=lambda x: x[1], reverse=True)
    
    print(f"{'Word':<25} | {'Uniformity (0-1)':<15} | {'Total Count'}")
    for w, e, c in noise_candidates[:30]:
        print(f"{w:<25} | {e:.4f}           | {c}")

if __name__ == "__main__":
    analyze_corpus(INPUT_FILE)
