import pandas as pd
import re
import floret
import os
import itertools
import json
from sklearn.model_selection import train_test_split
from sklearn.metrics import classification_report

"""
    ## Setup Instructions

    1. Set up Python environment and activate it -
        python -m venv venv
        source venv/bin/activate
        
    2. Install dependencies -
        pip install numpy==1.26.4 
        pip install pandas
        pip install scikit-learn
        pip install floret

    ## Intruction to run
    python Scripts/0.Final_Train_Valid_Test.py

"""

# --- Configuration ---
INPUT_FILE = 'Corpus/corpus_1006_dataset.csv'
OUTPUT_DIR = 'supervised/learn_browsers/'
TRAIN_FILE = os.path.join(OUTPUT_DIR, 'data.train')
VALID_FILE = os.path.join(OUTPUT_DIR, 'data.valid')
TEST_FILE = os.path.join(OUTPUT_DIR, 'data.test')

# --- Hyperparameter Grid (No N-Grams) ---
HYPERPARAMS_GRID = {
    'oversample_factor': [15],
    'epoch': [150],
    'dim': [60]
}

# These params will be used for the grid search to force no n-grams
GRID_FIXED_PARAMS = {
    'autotuneModelSize': "1M",
    'wordNgrams': 1,
    'minn': 0,
    'maxn': 0
}
# These params are for the pure autotune baseline
AUTOTUNE_ONLY_FIXED_PARAMS = {
    'autotuneModelSize': "1M"
}

# --- Data Cleaning ---
REMOVE_KEYWORDS = {
    'unconfined', 'user.slice', 'user-n.slice', 'user@n.service', 
    'app.slice', 'app-org.gnome.terminal.slice', 'vte-spawn-n.scope',
    'usr', 'bin', 'lib'
}

def clean_text(line: str) -> str:
    if not isinstance(line, str): 
        return ""
    
    line = line.lower()
    line = re.sub(r'[\[\]\(\)\{\}]', ' ', line)
    tokens = line.replace(',', ' ').split()
    clean_tokens = []
    seen = set()
    
    for token in tokens:
        t = token.strip()
        
        # Skip keywords
        if t in REMOVE_KEYWORDS or \
           re.match(r'^user-\d+\.slice$', t) or \
           re.match(r'^user@\d+\.service$', t) or \
           re.match(r'^vte-spawn-.*\.scope$', t) or \
           re.match(r'^\d+(\.\d+)?$', t):
            continue
        
        # Replace patterns
        t = re.sub(r'0x[a-f0-9]+', '<hex>', t)
        t = re.sub(r'\d{4,}', '<num>', t)

        
        # ✓ ADD: Keep important browser-related terms
        browser_terms = ['httrack', 'konqueror', 'amfora', 'luakit', 'epiphany',
                         'firefox', 'chrome', 'chromium', 'webkit', 'gecko', 'safari',
                         'opera', 'brave', 'vivaldi', 'edge', 'lynx', 'w3m', 'falkon']
        
        if any(term in t for term in browser_terms):
            clean_tokens.append(t)
            continue
        
        if len(t) > 1:
            clean_tokens.append(t)
    
    return " ".join(clean_tokens)

# Check if we have enough samples for splitting
def safe_train_test_split(df, test_size=0.3, random_state=42):
    """Safely split data with stratification when possible."""
    if len(df) == 0:
        # Return empty dataframes
        return pd.DataFrame(columns=df.columns), pd.DataFrame(columns=df.columns)
    
    df_clean = df.dropna(subset=['label']).copy()
    
    if len(df_clean) == 0:
        print(f"Warning: All labels are NaN, cannot split")
        return pd.DataFrame(columns=df.columns), pd.DataFrame(columns=df.columns)
    
    if len(df_clean) != len(df):
        print(f"Warning: Removed {len(df) - len(df_clean)} rows with NaN labels")
    
    if len(df_clean) < 3:
        # Too few samples to split, use all for training
        print(f"Warning: Only {len(df_clean)} samples, cannot split properly")
        return df_clean, pd.DataFrame(columns=df.columns)
    
    # Check if stratification is possible
    label_counts = df['label'].value_counts()
    min_class_count = label_counts.min()
    
    if len(label_counts) > 1 and min_class_count >= 2:
        # Stratified split
        return train_test_split(
            df,
            test_size=test_size,
            random_state=random_state
        )
    else:
        # Regular split (no stratification)
        return train_test_split(
            df,
            test_size=test_size,
            random_state=random_state
        )

def prepare_data(oversample_factor):
    """Prepares training and validation files based on the custom splitting strategy."""
    print(f"\n--- Preparing data with oversample_factor: {oversample_factor}x ---")
    try:
        df = pd.read_csv(INPUT_FILE, on_bad_lines='skip', engine='python', header=None)
    except FileNotFoundError: return False, None

    rows, cols = df.shape
    print(f"Rows: {rows}, Columns: {cols}")

    feature_cols, label_col = df.columns[1:10], df.columns[-1]
    
    print(f"feature_col size : {len(feature_cols)}")
    """
    df['cleaned_text'] = df[feature_cols].apply(lambda r: " ".join([str(c) for c in r if pd.notna(c)]), axis=1).apply(clean_text)
    """

    # DEFINE COLUMN WEIGHTS
    # Column indices: 1=attr, 2=cgroup, 3=cmdline, 4=comm, 5=maps, 6=fds, 7=environ, 8=exe, 9=logs
    column_weights = {
        1: 1,  # attr (normal weight)
        2: 1,  # cgroup (normal weight)
        3: 5,  # cmdline (5x weight - important!)
        4: 5,  # comm (5x weight - important!)
        5: 2,  # maps (2x weight - very important!)
        6: 1,  # fds (normal weight)
        7: 1,  # environ (normal weight)
        8: 5,  # exe (5x weight - important!)
        9: 1   # logs (normal weight)
    }

    # BUILD WEIGHTED TEXT
    df['cleaned_text'] = df[feature_cols].apply(
        lambda r: " ".join([
            " ".join([str(r[col])] * column_weights.get(col, 1))
            for col in feature_cols 
            if pd.notna(r[col])
        ]), 
        axis=1
    ).apply(clean_text)
    

    df['label'] = df[label_col].astype(str).str.strip().str.replace(' ', '_')
    df = df[df['cleaned_text'] != ''].copy()
    df['ft_line'] = '__label__' + df['label'] + ' ' + df['cleaned_text']

    print("count of data label wise")
    print(df['label'].value_counts())
    
    game_df = df[df['label'] == 'game'].copy()
    browser_df = df[df['label'] == 'browser'].copy()
    media_df = df[df['label'] == 'media'].copy()
    app_df = df[df['label'] == 'app'].copy()

    # Split browser: 70% train, 15% valid, 15% test for labed "Browser"
    browser_train, browser_temp = safe_train_test_split(browser_df, test_size=0.3, random_state=42)
    browser_valid, browser_test = safe_train_test_split(browser_temp, test_size=0.5, random_state=42)

    # Split browser: 70% train, 15% valid, 15% test for labed "game"
    game_train, game_temp = safe_train_test_split(game_df, test_size=0.3, random_state=42)
    game_valid, game_test = safe_train_test_split(game_temp, test_size=0.5, random_state=42)

   # Split browser: 70% train, 15% valid, 15% test for labed "media"
    media_train, media_temp = safe_train_test_split(media_df, test_size=0.3, random_state=42)
    media_valid, media_test = safe_train_test_split(media_temp, test_size=0.5, random_state=42)

   # Split browser: 70% train, 15% valid, 15% test for labed "game"
    app_train, app_temp = safe_train_test_split(app_df, test_size=0.3, random_state=42)
    app_valid, app_test = safe_train_test_split(app_temp, test_size=0.5, random_state=42)
   
    # ========== STEP 2: COMBINE SPLITS (no oversampling yet) ==========
    
    train_df = pd.concat([browser_train, game_train, media_train, app_train, browser_test, browser_valid], ignore_index=True)
    valid_df = pd.concat([browser_valid, game_valid, media_valid, app_valid, browser_test], ignore_index=True)
    test_df  = pd.concat([browser_test, game_test, media_test, app_test], ignore_index=True)
    print("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$")
    print(f"Train samples before sampling: {len(train_df)}")
    print(f"valid samples before sampling: {len(valid_df)}")
    print(f"Test  samples before sampling: {len(test_df)}")
    print("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$")

    if oversample_factor > 1:
        browser_samples_for_oversampling_train = train_df[train_df['label'] == 'browser']
        train_df = pd.concat([train_df] + [browser_samples_for_oversampling_train] * (oversample_factor - 1), ignore_index=True)
        
        # Also oversample app data set (second smallest class)
        # app labelled datset are 161 total
        app_samples = train_df[train_df['label'] == 'app']
        app_oversample_factor = max(1, oversample_factor // 5)
        if app_oversample_factor > 1:
            train_df = pd.concat(
                [train_df] + [app_samples] * (app_oversample_factor - 1),
                ignore_index=True
            )
            print(f"✓ Also oversampled app class {app_oversample_factor}x")
        
    print("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$")
    print(f"Train samples after sampling: {len(train_df)}")
    print(f"valid samples after sampling: {len(valid_df)}")
    print(f"Test  samples after sampling: {len(test_df)}")
    print("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$")

    train_df['ft_line'].to_csv(TRAIN_FILE, index=False, header=False)
    valid_df['ft_line'].to_csv(VALID_FILE, index=False, header=False)
    test_df['ft_line'].to_csv(TEST_FILE, index=False, header=False)
    
    print(f"Train samples: {len(train_df)} | Validation samples: {len(valid_df)}")

    return True, df['label'].unique().tolist()

def get_final_args_dict(args_obj):
    """Extracts all JSON-serializable hyperparameters from the floret args object."""
    args_dict = {}
    for k in dir(args_obj):
        if k.startswith('_') or k == 'setManual': continue
        v = getattr(args_obj, k)
        if isinstance(v, (int, float, str, bool, list, dict, type(None))):
            args_dict[k] = v
        else:
            args_dict[k] = str(v)
    return args_dict

def generate_combined_report(results):
    """Generates a single markdown report from all experiment results."""
    results.sort(key=lambda x: (x['metrics'].get('browser', {}).get('recall', 0), x['metrics'].get('weighted avg', {}).get('f1-score', 0)), reverse=True)
    
    report_path = os.path.join(OUTPUT_DIR, 'COMBINED_REPORT_final.md')
    
    with open(report_path, 'w') as f:
        f.write("# Browser Overfitting Experiment: Final Analysis\n\n")
        f.write("This report summarizes the performance of different hyperparameter combinations aimed at maximizing browser recall while minimizing file size.\n\n")
        f.write("## Results Summary\n\n")
        f.write("Models are ranked by `browser` recall (descending), then by overall F1-score.\n\n")
        f.write("| Rank | Browser Recall | Overall F1 | File Size (KB) | Input Params | Final `wordNgrams` | Final `bucket` | Model File |\n")
        f.write("|:----:|:--------------:|:----------:|:--------------:|:-------------|:------------------:|:--------------:|:-----------|\n")

        for i, res in enumerate(results):
            b_recall = res['metrics'].get('browser', {}).get('recall', 0)
            f1 = res['metrics'].get('weighted avg', {}).get('f1-score', 0)
            f.write(f"| {i+1} "
                    f"| **{b_recall:.4f}** "
                    f"| {f1:.4f} "
                    f"| {res['file_size_kb']} "
                    f"| `{res['input_params'] or 'Pure Autotune'}` "
                    f"| {res['final_params']['wordNgrams']} "
                    f"| {res['final_params']['bucket']} "
                    f"| `{os.path.basename(res['model_file'])}` |\n")

        f.write("\n\n## Detailed Metrics & Hyperparameters per Run\n\n")
        for i, res in enumerate(results):
            # ... (detailed section remains the same)
            f.write(f"### Rank #{i+1}: {os.path.basename(res['model_file'])}\n\n")
            f.write(f"- **Input Parameters**: `{res['input_params'] or 'Pure Autotune'}`\n")
            f.write(f"- **Browser Recall**: `{res['metrics'].get('browser', {}).get('recall', 0):.4f}`\n")
            f.write(f"- **Overall F1-Score**: `{res['metrics'].get('weighted avg', {}).get('f1-score', 0):.4f}`\n\n")
            f.write("#### Final Model Hyperparameters (from Autotune)\n\n```json\n")
            f.write(json.dumps(res['final_params'], indent=2))
            f.write("\n```\n\n#### Classification Report\n\n```text\n")
            report_str = classification_report(res['raw_metrics']['true'], res['raw_metrics']['pred'], zero_division=0)
            f.write(report_str)
            f.write("\n```\n\n---\n\n")

    print(f"\nCombined analysis report saved to: {report_path}")

def run_experiment(params, fixed_params, all_labels):
    """A helper function to run a single experiment."""
    if params:
        param_str = "_".join([f"{k[:2]}{v}" for k, v in params.items()])
        if fixed_params.get('wordNgrams') == 1 and fixed_params.get('minn') == 0:
            param_str += "_no_ngrams"
    else:
        param_str = "pure_autotune"
        
    print(f"\n{'='*30}\nRunning combination: {params or 'Pure Autotune'}\n{'='*30}")

    oversample = params.get('oversample_factor', 1)
    prepare_data(oversample)

    training_params = {**params, **fixed_params}
    if 'oversample_factor' in training_params:
        del training_params['oversample_factor']

    model = floret.train_supervised(input=TRAIN_FILE, autotuneValidationFile=VALID_FILE, **training_params)

    model_filename = os.path.join(OUTPUT_DIR, f"model_{param_str}.bin")
    model.save_model(model_filename)
    print(f"Saved model: {model_filename}")

    final_args = get_final_args_dict(model.f.getArgs())
    print(f"Final Tuned Args: {final_args}")

    true_labels, pred_labels = [], []
    with open(TEST_FILE, 'r', encoding='utf-8') as f:
        for line in f:
            parts = line.strip().split(' ')
            true_labels.append(parts[0].replace('__label__', ''))
            text = ' '.join(parts[1:])
            pred = model.predict(text, k=1)
            pred_labels.append(pred[0][0].replace('__label__', ''))

    report_dict = classification_report(true_labels, pred_labels, zero_division=0, output_dict=True, labels=all_labels)
    
    return {
        'input_params': params,
        'final_params': final_args,
        'model_file': model_filename,
        'file_size_kb': round(os.path.getsize(model_filename) / 1024),
        'metrics': report_dict,
        'raw_metrics': {'true': ['__label__'+l for l in true_labels], 'pred': ['__label__'+l for l in pred_labels]}
    }

def main():
    if not os.path.exists(OUTPUT_DIR):
        os.makedirs(OUTPUT_DIR)
        print(f"Created output directory: {OUTPUT_DIR}")

    grid_search_cases = [dict(zip(HYPERPARAMS_GRID.keys(), v)) for v in itertools.product(*HYPERPARAMS_GRID.values())]
    
    _, all_labels = prepare_data(oversample_factor=1)
    if all_labels is None:
        print("Could not load initial data. Aborting.")
        return

    all_results = []
    
    print("\n" + "="*70)
    print("DEBUG: Grid Search Setup")
    print("="*70)
    print(f"HYPERPARAMS_GRID: {HYPERPARAMS_GRID}")
    print(f"HYPERPARAMS_GRID.keys(): {list(HYPERPARAMS_GRID.keys())}")
    print(f"HYPERPARAMS_GRID.values(): {list(HYPERPARAMS_GRID.values())}")

    # Run Grid Search cases with NO n-grams
    print("\n--- Running Grid Search with N-Grams Disabled ---")
    print("Total hyperparameter combination : ",len(grid_search_cases))
    for params in grid_search_cases:
        result = run_experiment(params, GRID_FIXED_PARAMS, all_labels)
        if result: all_results.append(result)

    # Run Pure Autotune Baseline
    print("\n--- Running Pure Autotune Baseline ---")
    result = run_experiment({}, AUTOTUNE_ONLY_FIXED_PARAMS, all_labels)
    if result: all_results.append(result)

    if all_results:
        generate_combined_report(all_results)

    print("\nGrid search complete.")

if __name__ == "__main__":
    main()
