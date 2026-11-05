# Browser Overfitting Experiment: Final Analysis

This report summarizes the performance of different hyperparameter combinations aimed at maximizing browser recall while minimizing file size.

## Results Summary

Models are ranked by `browser` recall (descending), then by overall F1-score.

| Rank | Browser Recall | Overall F1 | File Size (KB) | Input Params | Final `wordNgrams` | Final `bucket` | Model File |
|:----:|:--------------:|:----------:|:--------------:|:-------------|:------------------:|:--------------:|:-----------|
| 1 | **1.0000** | 0.9145 | 267 | `Pure Autotune` | 1 | 0 | `model_pure_autotune.bin` |
| 2 | **1.0000** | 0.8983 | 293 | `{'oversample_factor': 15, 'epoch': 150, 'dim': 60}` | 1 | 0 | `model_ov15_ep150_di60_no_ngrams.bin` |


## Detailed Metrics & Hyperparameters per Run

### Rank #1: model_pure_autotune.bin

- **Input Parameters**: `Pure Autotune`
- **Browser Recall**: `1.0000`
- **Overall F1-Score**: `0.9145`

#### Final Model Hyperparameters (from Autotune)

```json
{
  "autotuneDuration": 300,
  "autotuneMetric": "f1",
  "autotuneModelSize": "1M",
  "autotunePredictions": 1,
  "autotuneValidationFile": "supervised/learn_browsers/data.valid",
  "bucket": 0,
  "cutoff": 53310,
  "dim": 90,
  "dsub": 16,
  "epoch": 100,
  "hashCount": 1,
  "input": "supervised/learn_browsers/data.train",
  "label": "__label__",
  "loss": "loss_name.softmax",
  "lr": 0.28316703694645484,
  "lrUpdateRate": 100,
  "maxn": 0,
  "minCount": 1,
  "minCountLabel": 0,
  "minn": 0,
  "mode": "mode_name.fasttext",
  "model": "model_name.supervised",
  "neg": 5,
  "output": "",
  "pretrainedVectors": "",
  "qnorm": true,
  "qout": false,
  "retrain": true,
  "saveOutput": false,
  "seed": 0,
  "t": 0.0001,
  "thread": 15,
  "verbose": 2,
  "wordNgrams": 1,
  "ws": 5
}
```

#### Classification Report

```text
                  precision    recall  f1-score   support

    __label__app       0.79      0.76      0.78        25
__label__browser       0.80      1.00      0.89         4
   __label__game       0.96      0.97      0.97        73
  __label__media       0.92      0.90      0.91        51

        accuracy                           0.92       153
       macro avg       0.87      0.91      0.89       153
    weighted avg       0.91      0.92      0.91       153

```

---

### Rank #2: model_ov15_ep150_di60_no_ngrams.bin

- **Input Parameters**: `{'oversample_factor': 15, 'epoch': 150, 'dim': 60}`
- **Browser Recall**: `1.0000`
- **Overall F1-Score**: `0.8983`

#### Final Model Hyperparameters (from Autotune)

```json
{
  "autotuneDuration": 300,
  "autotuneMetric": "f1",
  "autotuneModelSize": "1M",
  "autotunePredictions": 1,
  "autotuneValidationFile": "supervised/learn_browsers/data.valid",
  "bucket": 0,
  "cutoff": 36056,
  "dim": 60,
  "dsub": 4,
  "epoch": 150,
  "hashCount": 1,
  "input": "supervised/learn_browsers/data.train",
  "label": "__label__",
  "loss": "loss_name.softmax",
  "lr": 0.59405105601816,
  "lrUpdateRate": 100,
  "maxn": 0,
  "minCount": 1,
  "minCountLabel": 0,
  "minn": 0,
  "mode": "mode_name.fasttext",
  "model": "model_name.supervised",
  "neg": 5,
  "output": "",
  "pretrainedVectors": "",
  "qnorm": true,
  "qout": false,
  "retrain": true,
  "saveOutput": false,
  "seed": 0,
  "t": 0.0001,
  "thread": 15,
  "verbose": 2,
  "wordNgrams": 1,
  "ws": 5
}
```

#### Classification Report

```text
                  precision    recall  f1-score   support

    __label__app       0.82      0.92      0.87        25
__label__browser       0.57      1.00      0.73         4
   __label__game       0.98      0.89      0.94        73
  __label__media       0.87      0.88      0.87        51

        accuracy                           0.90       153
       macro avg       0.81      0.92      0.85       153
    weighted avg       0.91      0.90      0.90       153

```

---

