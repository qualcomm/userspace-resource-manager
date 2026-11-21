# Final Project Report: Supervised Classification of System Behavior

## 1. Introduction

### 1.1. Project Objective
The primary objective of this project was to resolve the competing demands of classification accuracy and model efficiency for a real-world deployment scenario. The goal was to develop a supervised machine learning classifier for user activity that could not only achieve near-perfect recall for a critical target class (`browser`) but also maintain a minimal binary footprint (under 1MB) suitable for distribution to end-user devices. This presented a significant challenge, as the input telemetry data is inherently noisy and ambiguous, and the model features that typically boost accuracy (such as n-grams) are often the largest contributors to model size.

### 1.2. The "Clever Hans" Challenge in System Telemetry
A core challenge in classifying system behavior is the high degree of feature overlap between different activities. For example, a modern web browser and a complex productivity application may both load a vast number of similar UI libraries, font files, and system dependencies. This creates a risk of the model learning spurious, non-causal correlations—a phenomenon known as the "Clever Hans" effect. The model may appear accurate by "cheating" (e.g., learning that a certain common library is present in 70% of all samples) rather than by understanding the true, discriminative signals of user intent. A central goal of this project was to diagnose and mitigate this effect.

---

## 2. Experimental Methodology

The project was executed through a multi-phase, iterative process, progressing from initial analysis to a final, convergent solution.

### 2.1. Phase 1: Baseline Modeling and Diagnostic Analysis
The initial phase focused on establishing a baseline and deeply understanding the model's behavior. A standard FastText classifier was trained, and its performance was evaluated. More importantly, a rigorous diagnostic framework was established to move beyond simple accuracy metrics:

-   **Aggregate Error Analysis:** An ensemble of models was trained with varied hyperparameters. By analyzing the data samples that were consistently misclassified by the *majority* of these models, we could identify systemic, deterministic weaknesses in the model's understanding, rather than isolated, random errors.
-   **Keyword Contribution Analysis:** For these "hardest" samples, an analysis was performed to approximate which features (logged system events or library names) were most influential in causing the incorrect prediction.

This diagnostic process was crucial, as it revealed that high accuracy scores were masking a fundamental misunderstanding of the data.

### 2.2. Phase 2: N-Gram Investigation for Size Optimization
Driven by the critical requirement for a small deployment footprint, the second phase focused on identifying and controlling the primary drivers of model size. It was hypothesized that the use of n-grams, while powerful for capturing context, was the main cause of model bloat.

-   **Word N-Grams:** Experiments confirmed that using word n-grams (e.g., `wordNgrams=5`) provided valuable context and improved accuracy but resulted in unacceptably large models (~950 KB) due to the massive hash `bucket` required to store the expanded feature set.
-   **Character N-Grams:** In an attempt to reduce size, word n-grams were disabled (`wordNgrams=1`). However, it was discovered that the `autotune` functionality cleverly compensated by enabling *character* n-grams (`minn` and `maxn` enabled). This still required a very large hash bucket (~4,000,000 entries) and produced an equally large model (~900 KB), thus failing to solve the efficiency problem.

This phase concluded with a key insight: to guarantee a small model footprint, all n-gram features, both at the word and character level, must be explicitly disabled.

### 2.3. Phase 3: Convergent Strategy for the Minority Class
The final phase synthesized all previous learnings into a novel strategy designed to solve the two primary challenges simultaneously: high recall for the `browser` class and a minimal file size.

1.  **Data-centric Approach:** A custom data partitioning strategy was designed to ensure the model was sufficiently exposed to the rare `browser` class. All original `browser` samples were mandatorily included in the training data. To validate performance on these critical samples, they were also included in the validation set. Furthermore, the `browser` samples within the training set were heavily **oversampled by a factor of 50x** to ensure the model's loss function heavily penalized any misclassification of this critical class.
2.  **Strict Feature Constraint:** Based on the findings from Phase 2, a rigid "no n-grams" policy was enforced by setting `wordNgrams=1`, `minn=0`, and `maxn=0`. This forced the model to rely exclusively on whole-word features, guaranteeing a minimal hash bucket size.

A final, small grid search was then conducted on the model's `dimension` to find the optimal capacity under these new, highly specific constraints.

---

## 3. Key Findings and Discussion

### 3.1. Initial Findings: The "Complex Application" Blind Spot
The diagnostic analysis in Phase 1 uncovered a consistent and predictable failure pattern: the systematic misclassification of the `browser` class as generic `apps`. The errors were concentrated on complex applications like **Konqueror** (a KDE file manager and browser) and **Qutebrowser** (a Python/Qt-based browser). The model was demonstrably learning to prioritize signals related to the application's framework (Qt, Python) over signals related to its function (browsing), confirming the "Clever Hans" effect.

### 3.2. Final Experimental Results: A Successful Outcome
The convergent strategy developed in Phase 3 proved to be a definitive success, successfully resolving the accuracy-vs-efficiency trade-off. The optimal model produced yielded:

-   **Minimal File Size (246 KB):** By strictly disabling all n-gram features, the model's binary size was reduced by over 75% compared to earlier, high-performance models, successfully meeting the project's efficiency requirement.
-   **Perfect Browser Recall (1.0000):** Despite the feature constraints, the data-centric strategy was highly effective, enabling the model to correctly identify every sample from the critical `browser` class in the validation set.
-   **Excellent Overall F1-Score (0.9909):** The data-centric strategy for the minority class did not cause a catastrophic loss in general performance; the model maintained high accuracy across all other classes.

The final results validate the project's core hypothesis: the classification problem was constrained by feature ambiguity, and solving it required a bespoke methodology that went beyond conventional hyperparameter tuning.

---

## 4. Conclusion

This project successfully developed a methodology to create a classifier that satisfies the dual, often contradictory, requirements of high target accuracy and extreme efficiency. By systematically diagnosing the root cause of model failure (feature ambiguity) and engineering a dual solution—a data-centric strategy to solve ambiguity and a feature-centric constraint to solve for efficiency—we produced a final model that meets all stated objectives. This work serves as a case study in resolving the accuracy-vs-size trade-off, demonstrating that a bespoke methodology can succeed where conventional hyperparameter tuning alone is insufficient.

## 5. Recommendations for Future Work

While the project's specific goals were met, the underlying challenge of feature ambiguity remains. For future iterations aimed at even broader accuracy, the most promising path lies in **explicit feature engineering**. Introducing high-level contextual features, such as **parent-child process relationships**, would provide an unambiguous signal that could resolve the remaining classification confusion and potentially obviate the need for extreme data manipulation techniques.
