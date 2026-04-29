#pragma once
#include "types.h"

// ---------------------------------------------
//  MCQ TRIAGE QUESTIONNAIRE
// ---------------------------------------------

struct Question {
    string text;
    vector<string> options;
    vector<int> scores;  // Each option contributes severity score
};

// Score thresholds -> Priority
//   >= 8 -> RED, >=5 -> ORANGE, >=3 -> YELLOW, else GREEN
Priority scoreToTriage(int score) {
    if (score >= 8) return Priority::P_RED;
    if (score >= 5) return Priority::P_ORANGE;
    if (score >= 3) return Priority::P_YELLOW;
    return Priority::P_GREEN;
}

int askMCQ(const Question& q) {
    cout << "\n  " << BOLD << q.text << RESET << "\n";
    for (int i = 0; i < (int)q.options.size(); i++) {
        cout << "    " << CYAN << (i+1) << RESET << ". " << q.options[i] << "\n";
    }
    int choice = 1;
    readIntInRange("  Your choice [1-" + to_string(q.options.size()) + "]: ",
                   choice, 1, (int)q.options.size());
    return q.scores[choice - 1];
}

EmergencyRequest collectPatientInfo(int id, const Graph& g) {
    EmergencyRequest req;
    req.id = id;

    printHeader("PATIENT INTAKE FORM");

    cout << "\n  Enter patient name: ";
    getline(cin, req.patientName);
    if (req.patientName.empty()) req.patientName = "Unknown";

    cout << "\n  Available locations:\n";
    for (auto& kv : g.nodeNames)
        cout << "    " << CYAN << kv.first << RESET << " - " << kv.second << "\n";
    readIntInRange("\n  Enter location node: ", req.locationNode, 0, g.V - 1);

    // -- Triage MCQ ----------------------------------
    printSubHeader("TRIAGE QUESTIONNAIRE  (answer all questions)");

    vector<Question> questions = {
        {
            "1. What is the primary complaint?",
            {"Chest pain / difficulty breathing",
             "Severe bleeding / trauma",
             "Loss of consciousness / seizure",
             "Moderate pain or injury",
             "Mild pain, fever, or nausea"},
            {4, 3, 4, 2, 1}
        },
        {
            "2. Is the patient conscious and responding?",
            {"Unconscious / unresponsive",
             "Conscious but confused",
             "Conscious and alert"},
            {4, 2, 0}
        },
        {
            "3. How is the patient's breathing?",
            {"Not breathing / very laboured",
             "Rapid / shallow",
             "Normal"},
            {4, 2, 0}
        },
        {
            "4. Is there visible severe bleeding?",
            {"Yes - uncontrolled",
             "Yes - controlled/minor",
             "No"},
            {3, 1, 0}
        },
        {
            "5. Does the patient have a known heart condition or stroke history?",
            {"Yes",
             "Unknown",
             "No"},
            {2, 1, 0}
        },
        {
            "6. How long have symptoms been present?",
            {"Just started (< 5 minutes)",
             "< 1 hour",
             "Several hours",
             "More than a day"},
            {3, 2, 1, 0}
        }
    };

    int totalScore = 0;
    for (auto& q : questions)
        totalScore += askMCQ(q);

    req.priority = scoreToTriage(totalScore);

    cout << "\n";
    printLine("-");
    cout << "  Triage Score: " << BOLD << totalScore << RESET << "\n";
    cout << "  Assigned Priority: ";
    printPriority(req.priority);
    cout << " " << priorityName(req.priority) << RESET << "\n";
    printLine("-");

    // Build symptom summary
    req.symptoms = "Triage score: " + to_string(totalScore) +
                   " -> " + priorityName(req.priority);
    req.status = "PENDING";
    return req;
}
