import pandas as pd
import os

# Configuration
INPUT_FILE = 'supervised/corpus.csv'

def analyze_distribution():
    """
    Reads the corpus file and prints the distribution of the label column.
    """
    print(f"--- Analyzing Class Distribution in {INPUT_FILE} ---")
    
    if not os.path.exists(INPUT_FILE):
        print(f"Error: Corpus file not found at '{INPUT_FILE}'")
        return

    try:
        df = pd.read_csv(INPUT_FILE, on_bad_lines='skip', engine='python', header=None)
    except Exception as e:
        print(f"Error reading CSV file: {e}")
        return

    if df.empty or len(df.columns) < 1:
        print("Error: The CSV file is empty or formatted incorrectly.")
        return
        
    # The label is in the last column
    label_col_index = df.columns[-1]
    df['label'] = df[label_col_index].astype(str).str.strip().str.replace(' ', '_')

    print("\nClass Counts:")
    class_counts = df['label'].value_counts()
    print(class_counts)
    
    print("\nClass Percentages:")
    class_percentages = df['label'].value_counts(normalize=True) * 100
    print(class_percentages.round(2).astype(str) + '%')
    
    print(f"\nTotal samples: {len(df)}")

if __name__ == "__main__":
    analyze_distribution()
