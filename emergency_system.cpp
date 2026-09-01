/*
 * ============================================================================
 *  SMART EMERGENCY MEDICAL RESOURCE COORDINATION SYSTEM
 *  Language   : C++17
 *  Model      : Dynamic Emergency Resource Matching Model
 *
 *  Team:
 *    Member 1 - Emergency Case & Priority Engine
 *    Member 2 - Resource Architecture & Allocation Engine
 *    Member 3 - Coordination, Tracking & Reporting Engine
 * ============================================================================
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <limits>
#include <cmath>
#include <algorithm>

using namespace std;

/* ============================================================================
 *  GLOBAL CONSTANTS
 * ============================================================================ */
const int MAX_PATIENTS  = 50;
const int MAX_CASES     = 50;
const int MAX_RESOURCES = 30;

/* ============================================================================
 *  ENUMS
 * ============================================================================ */
enum class ResourceCategory { AMBULANCE, BED, EQUIPMENT, UNKNOWN };
enum class CaseStatus       { PENDING, ALLOCATED, COMPLETED };

string categoryToString(ResourceCategory c) {
    switch (c) {
        case ResourceCategory::AMBULANCE: return "AMBULANCE";
        case ResourceCategory::BED:       return "BED";
        case ResourceCategory::EQUIPMENT: return "EQUIPMENT";
        default:                          return "UNKNOWN";
    }
}

string statusToString(CaseStatus s) {
    switch (s) {
        case CaseStatus::PENDING:   return "PENDING";
        case CaseStatus::ALLOCATED: return "ALLOCATED";
        case CaseStatus::COMPLETED: return "COMPLETED";
    }
    return "UNKNOWN";
}

/* ============================================================================
 *  MEMBER 1 : Patient  (Emergency Case & Priority Engine)
 * ============================================================================ */
class Patient {
private:
    int    patientID;
    string name;
    int    age;
    string contactNumber;
    bool   valid; // true once properly initialised

public:
    // Default constructor
    Patient() : patientID(-1), name(""), age(0), contactNumber(""), valid(false) {}

    // Parameterized constructor
    Patient(int id, const string& n, int a, const string& contact)
        : patientID(id), name(n), age(a), contactNumber(contact), valid(true) {}

    // Copy constructor
    Patient(const Patient& other)
        : patientID(other.patientID), name(other.name), age(other.age),
          contactNumber(other.contactNumber), valid(other.valid) {}

    int getID() const { return patientID; }
    string getName() const { return name; }
    int getAge() const { return age; }
    string getContact() const { return contactNumber; }
    bool isValid() const { return valid; }

    void display() const {
        cout << "  Patient ID: " << patientID
             << " | Name: " << name
             << " | Age: " << age
             << " | Contact: " << contactNumber << "\n";
    }
};

/* ============================================================================
 *  MEMBER 1 : EmergencyCase
 *
 *  Emergency Coordination Score (ECS) formula (documented in Section 5 of the
 *  project report, Algorithm 3):
 *
 *      ECS = UrgencyComponent + WaitingComponent + ResourceNeedComponent
 *            - DelayPenalty
 *
 *      UrgencyComponent    = urgencyLevel * 10                (urgency 1-10)
 *      WaitingComponent    = min(waitingTimeMinutes, 60) * 0.5
 *      ResourceNeedComponent =
 *            AMBULANCE -> 15 , BED -> 10 , EQUIPMENT -> 8
 *      DelayPenalty        = (waitingTimeMinutes > 120) ?
 *                              (waitingTimeMinutes - 120) * 0.2 : 0
 *
 *  Range: urgencyLevel in [1,10], waitingTimeMinutes >= 0.
 *  Higher ECS  =>  higher priority case.
 * ============================================================================ */
class EmergencyCase {
private:
    int caseID;
    int patientID;
    int urgencyLevel;              // 1 (low) .. 10 (critical)
    int waitingTimeMinutes;        // minutes since case was opened
    ResourceCategory requiredCategory;
    int requiredCapabilityLevel;   // minimum capability level needed (1-5)
    CaseStatus status;
    int assignedResourceID;        // -1 if none
    double ecsScore;

public:
    // Default constructor
    EmergencyCase()
        : caseID(-1), patientID(-1), urgencyLevel(1), waitingTimeMinutes(0),
          requiredCategory(ResourceCategory::UNKNOWN), requiredCapabilityLevel(1),
          status(CaseStatus::PENDING), assignedResourceID(-1), ecsScore(0.0) {}

    // Parameterized constructor
    EmergencyCase(int cID, int pID, int urgency, int waitMinutes,
                  ResourceCategory category, int capLevel)
        : caseID(cID), patientID(pID), urgencyLevel(urgency),
          waitingTimeMinutes(waitMinutes), requiredCategory(category),
          requiredCapabilityLevel(capLevel), status(CaseStatus::PENDING),
          assignedResourceID(-1), ecsScore(0.0) {
        calculateECS();
    }

    // Copy constructor
    EmergencyCase(const EmergencyCase& other)
        : caseID(other.caseID), patientID(other.patientID),
          urgencyLevel(other.urgencyLevel), waitingTimeMinutes(other.waitingTimeMinutes),
          requiredCategory(other.requiredCategory),
          requiredCapabilityLevel(other.requiredCapabilityLevel),
          status(other.status), assignedResourceID(other.assignedResourceID),
          ecsScore(other.ecsScore) {}

    // ---- Algorithm 3 : ECS Calculation ----
    void calculateECS() {
        double urgencyComponent = urgencyLevel * 10.0;
        double waitingComponent = min(waitingTimeMinutes, 60) * 0.5;

        double resourceNeedComponent = 0.0;
        switch (requiredCategory) {
            case ResourceCategory::AMBULANCE: resourceNeedComponent = 15.0; break;
            case ResourceCategory::BED:       resourceNeedComponent = 10.0; break;
            case ResourceCategory::EQUIPMENT: resourceNeedComponent = 8.0;  break;
            default: resourceNeedComponent = 0.0; break;
        }

        double delayPenalty = 0.0;
        if (waitingTimeMinutes > 120) {
            delayPenalty = (waitingTimeMinutes - 120) * 0.2;
        }

        ecsScore = urgencyComponent + waitingComponent + resourceNeedComponent - delayPenalty;
    }

    void addWaitingMinutes(int minutes) {
        waitingTimeMinutes += minutes;
        calculateECS();
    }

    // ---- Operator 1 : compare two cases by ECS (higher ECS = higher priority) ----
    bool operator>(const EmergencyCase& other) const {
        return this->ecsScore > other.ecsScore;
    }
    bool operator<(const EmergencyCase& other) const {
        return this->ecsScore < other.ecsScore;
    }

    // ---- Operator 2 : formatted stream output ----
    friend ostream& operator<<(ostream& os, const EmergencyCase& ec) {
        os << "  Case #" << ec.caseID
           << " | Patient ID: " << ec.patientID
           << " | Urgency: " << ec.urgencyLevel << "/10"
           << " | Waiting: " << ec.waitingTimeMinutes << " min"
           << " | Needs: " << categoryToString(ec.requiredCategory)
           << " (Level " << ec.requiredCapabilityLevel << ")"
           << " | ECS: " << fixed << setprecision(2) << ec.ecsScore
           << " | Status: " << statusToString(ec.status);
        if (ec.assignedResourceID != -1)
            os << " | Resource: R" << ec.assignedResourceID;
        return os;
    }

    // Accessors
    int getCaseID() const { return caseID; }
    int getPatientID() const { return patientID; }
    int getUrgency() const { return urgencyLevel; }
    int getWaitingTime() const { return waitingTimeMinutes; }
    ResourceCategory getRequiredCategory() const { return requiredCategory; }
    int getRequiredCapabilityLevel() const { return requiredCapabilityLevel; }
    CaseStatus getStatus() const { return status; }
    int getAssignedResourceID() const { return assignedResourceID; }
    double getECS() const { return ecsScore; }

    void setStatus(CaseStatus s) { status = s; }
    void setAssignedResourceID(int id) { assignedResourceID = id; }
};

/* ============================================================================
 *  MEMBER 2 : Resource Architecture
 *
 *  Diamond inheritance:
 *
 *                    ResourceProfile  (abstract)
 *                          |
 *              +-----------+-----------+
 *              |                       |
 *      TransportCapability     ClinicalCapability
 *              |                       |
 *              +-----------+-----------+
 *                          |
 *                EmergencyResource  (abstract, multiple + virtual inheritance)
 *                    /       |        \
 *             Ambulance  EmergencyBed  MedicalEquipment
 *
 *  Every concrete resource in this hospital simultaneously needs a mobility
 *  profile (can it be moved / how fast can it reach the patient) AND a
 *  clinical profile (what medical capability level does it provide). Both
 *  TransportCapability and ClinicalCapability inherit from the same
 *  ResourceProfile base (id, category, zone). Without virtual inheritance,
 *  EmergencyResource would contain two separate copies of ResourceProfile's
 *  data members (resourceID, category, zone) - one via TransportCapability
 *  and one via ClinicalCapability - causing ambiguity such as
 *  "resource.resourceID" being illegal (compiler cannot tell which copy).
 *  `virtual` inheritance on both intermediate classes guarantees a SINGLE
 *  shared ResourceProfile sub-object inside EmergencyResource.
 * ============================================================================ */

// ---- Abstract base ----
class ResourceProfile {
protected:
    int resourceID;
    ResourceCategory category;
    string zone;   // hospital / location zone

public:
    ResourceProfile() : resourceID(-1), category(ResourceCategory::UNKNOWN), zone("") {}
    ResourceProfile(int id, ResourceCategory cat, const string& z)
        : resourceID(id), category(cat), zone(z) {}

    virtual ~ResourceProfile() {}   // virtual destructor - polymorphic base

    int getResourceID() const { return resourceID; }
    ResourceCategory getCategory() const { return category; }
    string getZone() const { return zone; }

    // Pure virtual function - every resource must describe itself
    virtual string describe() const = 0;
};

// ---- Intermediate 1 : mobility profile ----
class TransportCapability : virtual public ResourceProfile {
protected:
    int responseTimeMinutes; // time to reach the patient

public:
    TransportCapability() : responseTimeMinutes(0) {}
    TransportCapability(int respTime) : responseTimeMinutes(respTime) {}
    int getResponseTime() const { return responseTimeMinutes; }
};

// ---- Intermediate 2 : clinical profile ----
class ClinicalCapability : virtual public ResourceProfile {
protected:
    int capabilityLevel; // 1 (basic) .. 5 (advanced / critical care)

public:
    ClinicalCapability() : capabilityLevel(1) {}
    ClinicalCapability(int level) : capabilityLevel(level) {}
    int getCapabilityLevel() const { return capabilityLevel; }
};

// ---- Diamond join : EmergencyResource ----
class EmergencyResource : public TransportCapability, public ClinicalCapability {
protected:
    int    currentWorkload;   // number of active cases currently assigned (0..N)
    bool   available;         // true if free to allocate
    int    totalAllocations;  // utilization tracking - times this resource was used
    int    totalBusyMinutes;  // utilization tracking - cumulative busy time

public:
    EmergencyResource()
        : currentWorkload(0), available(true), totalAllocations(0), totalBusyMinutes(0) {}

    EmergencyResource(int id, ResourceCategory cat, const string& zone,
                       int respTime, int capLevel)
        : ResourceProfile(id, cat, zone),
          TransportCapability(respTime),
          ClinicalCapability(capLevel),
          currentWorkload(0), available(true),
          totalAllocations(0), totalBusyMinutes(0) {}

    virtual ~EmergencyResource() {}

    bool isAvailable() const { return available; }
    int getWorkload() const { return currentWorkload; }
    int getTotalAllocations() const { return totalAllocations; }
    int getTotalBusyMinutes() const { return totalBusyMinutes; }

    void markAllocated() {
        available = false;
        currentWorkload++;
        totalAllocations++;
    }

    void markReleased(int busyMinutes) {
        available = true;
        if (currentWorkload > 0) currentWorkload--;
        totalBusyMinutes += busyMinutes;
    }

    /* ---- Algorithm 4 : Resource Suitability Calculation ----
     *
     *  Suitability = CapabilityScore + AvailabilityBonus
     *                - WorkloadPenalty - ResponseTimePenalty
     *
     *  CapabilityScore    = capabilityLevel * 10           (range 10-50)
     *  AvailabilityBonus  = available ? 20 : 0
     *  WorkloadPenalty    = currentWorkload * 5
     *  ResponseTimePenalty= responseTimeMinutes * 1.0
     *
     *  A resource whose capabilityLevel is below the case's required level
     *  is considered UNSUITABLE and returns a large negative score so it is
     *  never selected (Algorithm 5, Step 4: capability sufficiency check).
     */
    virtual double calculateSuitability(int requiredCapabilityLevel) const {
        if (capabilityLevel < requiredCapabilityLevel) return -1000.0; // insufficient capability
        if (!available) return -1000.0;

        double capabilityScore   = capabilityLevel * 10.0;
        double availabilityBonus = available ? 20.0 : 0.0;
        double workloadPenalty   = currentWorkload * 5.0;
        double responsePenalty   = responseTimeMinutes * 1.0;

        return capabilityScore + availabilityBonus - workloadPenalty - responsePenalty;
    }

    // Pure virtual from ResourceProfile - each concrete resource implements it
    string describe() const override = 0;
};

// ---- Concrete resources (Member 2) ----
class Ambulance : public EmergencyResource {
public:
    Ambulance() : ResourceProfile(), EmergencyResource() {}
    // NOTE: Ambulance is the most-derived class, so C++ requires IT (not
    // EmergencyResource) to initialise the shared virtual base ResourceProfile
    // directly. If this explicit call is omitted, ResourceProfile is silently
    // default-constructed and resourceID/category/zone are lost - this was
    // caught during our own test execution (see Section 10 of the report).
    Ambulance(int id, const string& zone, int respTime, int capLevel)
        : ResourceProfile(id, ResourceCategory::AMBULANCE, zone),
          EmergencyResource(id, ResourceCategory::AMBULANCE, zone, respTime, capLevel) {}

    string describe() const override {
        return "Ambulance R" + to_string(resourceID) + " [Zone: " + zone +
               ", Capability: " + to_string(capabilityLevel) +
               ", Response: " + to_string(responseTimeMinutes) + " min]";
    }
};

class EmergencyBed : public EmergencyResource {
public:
    EmergencyBed() : ResourceProfile(), EmergencyResource() {}
    EmergencyBed(int id, const string& zone, int capLevel)
        : ResourceProfile(id, ResourceCategory::BED, zone),
          EmergencyResource(id, ResourceCategory::BED, zone, /*respTime*/0, capLevel) {}

    string describe() const override {
        return "Emergency Bed R" + to_string(resourceID) + " [Zone: " + zone +
               ", Capability: " + to_string(capabilityLevel) + "]";
    }
};

class MedicalEquipment : public EmergencyResource {
private:
    string equipmentName;
public:
    MedicalEquipment() : ResourceProfile(), EmergencyResource() {}
    MedicalEquipment(int id, const string& zone, int respTime, int capLevel, const string& name)
        : ResourceProfile(id, ResourceCategory::EQUIPMENT, zone),
          EmergencyResource(id, ResourceCategory::EQUIPMENT, zone, respTime, capLevel),
          equipmentName(name) {}

    string describe() const override {
        return "Equipment R" + to_string(resourceID) + " (" + equipmentName + ") [Zone: " + zone +
               ", Capability: " + to_string(capabilityLevel) +
               ", Response: " + to_string(responseTimeMinutes) + " min]";
    }
};

/* ============================================================================
 *  MEMBER 3 : Coordination, Tracking & Reporting Engine
 * ============================================================================ */
class CoordinationSystem {
private:
    Patient patients[MAX_PATIENTS];
    int patientCount;

    EmergencyCase cases[MAX_CASES];
    int caseCount;

    // Dynamic memory: base-class pointers -> runtime polymorphism
    vector<EmergencyResource*> resources;

    // Static members - genuinely shared across the whole system, not per-object
    static int totalRegisteredResources;
    static int totalEmergencyCases;
    static int totalActiveCases;
    static int totalAllocationsMade;
    static int nextCaseID;

public:
    CoordinationSystem() : patientCount(0), caseCount(0) {}

    // Destructor - releases dynamically allocated resources (Member 3 memory mgmt)
    ~CoordinationSystem() {
        for (auto* r : resources) delete r;
        resources.clear();
    }

    /* ---------------- Algorithm 1 : Patient Registration ---------------- */
    bool registerPatient(int id, const string& name, int age, const string& contact) {
        if (patientCount >= MAX_PATIENTS) {
            cout << "  [ERROR] Patient capacity reached.\n";
            return false;
        }
        for (int i = 0; i < patientCount; i++) {
            if (patients[i].getID() == id) {
                cout << "  [ERROR] Duplicate Patient ID.\n";
                return false;
            }
        }
        patients[patientCount++] = Patient(id, name, age, contact);
        cout << "  [OK] Patient registered successfully.\n";
        return true;
    }

    int findPatientIndex(int id) const {
        for (int i = 0; i < patientCount; i++)
            if (patients[i].getID() == id) return i;
        return -1;
    }

    /* ---------------- Resource Registration ---------------- */
    bool registerResource(EmergencyResource* r) {
        if ((int)resources.size() >= MAX_RESOURCES) {
            cout << "  [ERROR] Resource capacity reached.\n";
            delete r;
            return false;
        }
        for (auto* existing : resources) {
            if (existing->getResourceID() == r->getResourceID()) {
                cout << "  [ERROR] Duplicate Resource ID.\n";
                delete r;
                return false;
            }
        }
        resources.push_back(r);
        totalRegisteredResources++;
        cout << "  [OK] Resource registered: " << r->describe() << "\n";
        return true;
    }

    /* ---------------- Algorithm 2 : Emergency Case Creation ---------------- */
    bool createEmergencyCase(int patientID, int urgency, int waitMinutes,
                              ResourceCategory category, int capLevel) {
        if (caseCount >= MAX_CASES) {
            cout << "  [ERROR] Case capacity reached.\n";
            return false;
        }
        if (findPatientIndex(patientID) == -1) {
            cout << "  [ERROR] Invalid Patient ID. Register the patient first.\n";
            return false;
        }
        if (urgency < 1 || urgency > 10) {
            cout << "  [ERROR] Invalid urgency value (must be 1-10).\n";
            return false;
        }
        int cID = nextCaseID++;
        cases[caseCount++] = EmergencyCase(cID, patientID, urgency, waitMinutes, category, capLevel);
        totalEmergencyCases++;
        totalActiveCases++;
        cout << "  [OK] Emergency case #" << cID << " created.\n";
        return true;
    }

    int findCaseIndex(int caseID) const {
        for (int i = 0; i < caseCount; i++)
            if (cases[i].getCaseID() == caseID) return i;
        return -1;
    }

    void viewPendingCases() const {
        bool any = false;
        cout << "\n  --- Pending Emergency Cases (sorted by ECS priority) ---\n";
        vector<int> idx;
        for (int i = 0; i < caseCount; i++)
            if (cases[i].getStatus() == CaseStatus::PENDING) idx.push_back(i);

        sort(idx.begin(), idx.end(), [this](int a, int b) {
            return cases[a] > cases[b]; // operator> uses ECS
        });

        for (int i : idx) { cout << cases[i] << "\n"; any = true; }
        if (!any) cout << "  (no pending cases)\n";
    }

    /* ---------------- Algorithm 5 : Best Resource Allocation ---------------- */
    bool allocateBestResource(int caseID) {
        int ci = findCaseIndex(caseID);
        if (ci == -1) { cout << "  [ERROR] Invalid Case ID.\n"; return false; }
        EmergencyCase& ec = cases[ci];

        if (ec.getStatus() != CaseStatus::PENDING) {
            cout << "  [ERROR] Case is not pending (already " << statusToString(ec.getStatus()) << ").\n";
            return false;
        }

        EmergencyResource* best = nullptr;   // base-class pointer -> polymorphism
        double bestScore = -1000.0;

        for (auto* r : resources) {
            if (r->getCategory() != ec.getRequiredCategory()) continue; // Step 2/3
            double score = r->calculateSuitability(ec.getRequiredCapabilityLevel()); // Step 5-8 (virtual call)
            if (score > bestScore) {
                bestScore = score;
                best = r;
            }
        }

        if (best == nullptr || bestScore <= -1000.0) {
            cout << "  [PENDING] No suitable available resource found for Case #" << caseID
                 << " (" << categoryToString(ec.getRequiredCategory())
                 << ", capability >= " << ec.getRequiredCapabilityLevel() << "). "
                 << "Case remains pending.\n";
            return false;
        }

        best->markAllocated();
        ec.setStatus(CaseStatus::ALLOCATED);
        ec.setAssignedResourceID(best->getResourceID());
        totalAllocationsMade++;

        cout << "  [ALLOCATED] " << best->describe()
             << " assigned to Case #" << caseID
             << " (Suitability Score: " << fixed << setprecision(2) << bestScore << ")\n";
        return true;
    }

    /* ---------------- Resource status update ---------------- */
    void updateResourceStatus() const {
        cout << "\n  --- Resource Status ---\n";
        for (auto* r : resources) {
            cout << "  " << r->describe()
                 << " | Available: " << (r->isAvailable() ? "YES" : "NO")
                 << " | Workload: " << r->getWorkload() << "\n";
        }
    }

    /* ---------------- Algorithm 6 : Resource Release / Case Completion ---------------- */
    bool completeEmergencyCase(int caseID, int busyMinutes) {
        int ci = findCaseIndex(caseID);
        if (ci == -1) { cout << "  [ERROR] Invalid Case ID.\n"; return false; }
        EmergencyCase& ec = cases[ci];

        if (ec.getStatus() != CaseStatus::ALLOCATED) {
            cout << "  [ERROR] Case is not currently allocated.\n";
            return false;
        }

        for (auto* r : resources) {
            if (r->getResourceID() == ec.getAssignedResourceID()) {
                r->markReleased(busyMinutes);
                break;
            }
        }

        ec.setStatus(CaseStatus::COMPLETED);
        totalActiveCases--;
        cout << "  [OK] Case #" << caseID << " marked COMPLETED. Resource released.\n";
        return true;
    }

    /* ---------------- Algorithm 8 : Case Comparison ---------------- */
    void compareCases(int id1, int id2) const {
        int i1 = findCaseIndex(id1), i2 = findCaseIndex(id2);
        if (i1 == -1 || i2 == -1) { cout << "  [ERROR] Invalid Case ID(s).\n"; return; }
        cout << "\n  " << cases[i1] << "\n  " << cases[i2] << "\n";
        if (cases[i1] > cases[i2])
            cout << "  => Case #" << id1 << " has HIGHER priority (ECS-based).\n";
        else if (cases[i2] > cases[i1])
            cout << "  => Case #" << id2 << " has HIGHER priority (ECS-based).\n";
        else
            cout << "  => Both cases have EQUAL priority.\n";
    }

    /* ---------------- Algorithm 7 : Resource Utilization Calculation ---------------- */
    void viewResourceUtilization() const {
        cout << "\n  --- Resource Utilization Report ---\n";
        for (auto* r : resources) {
            double utilization = 0.0;
            if (r->getTotalAllocations() > 0)
                utilization = (double)r->getTotalBusyMinutes() / r->getTotalAllocations();
            cout << "  " << r->describe()
                 << " | Allocations: " << r->getTotalAllocations()
                 << " | Total Busy Minutes: " << r->getTotalBusyMinutes()
                 << " | Avg Busy Time/Allocation: " << fixed << setprecision(2) << utilization << " min\n";
        }
    }

    /* ---------------- Algorithm 9 : Coordination Report ---------------- */
    void generateCoordinationReport() const {
        cout << "\n  ======================= COORDINATION REPORT =======================\n";
        cout << "  Total Registered Resources : " << totalRegisteredResources << "\n";
        cout << "  Total Emergency Cases      : " << totalEmergencyCases << "\n";
        cout << "  Active (unfinished) Cases  : " << totalActiveCases << "\n";
        cout << "  Total Allocations Made     : " << totalAllocationsMade << "\n";

        int pending = 0, allocated = 0, completed = 0;
        for (int i = 0; i < caseCount; i++) {
            switch (cases[i].getStatus()) {
                case CaseStatus::PENDING:   pending++;   break;
                case CaseStatus::ALLOCATED: allocated++; break;
                case CaseStatus::COMPLETED: completed++; break;
            }
        }
        cout << "  Pending: " << pending << " | Allocated: " << allocated
             << " | Completed: " << completed << "\n";
        cout << "  =====================================================================\n";
    }

    void displaySystemStatistics() const {
        cout << "\n  --- System Statistics ---\n";
        cout << "  Registered Patients   : " << patientCount << "\n";
        cout << "  Registered Resources  : " << (int)resources.size() << "\n";
        cout << "  Total Emergency Cases : " << totalEmergencyCases << "\n";
        cout << "  Total Allocations     : " << totalAllocationsMade << "\n";
    }

    void listPatients() const {
        cout << "\n  --- Registered Patients ---\n";
        if (patientCount == 0) { cout << "  (none)\n"; return; }
        for (int i = 0; i < patientCount; i++) patients[i].display();
    }

    void listResources() const {
        cout << "\n  --- Registered Resources ---\n";
        if (resources.empty()) { cout << "  (none)\n"; return; }
        for (auto* r : resources) cout << "  " << r->describe() << "\n";
    }
};

// Static member definitions
int CoordinationSystem::totalRegisteredResources = 0;
int CoordinationSystem::totalEmergencyCases = 0;
int CoordinationSystem::totalActiveCases = 0;
int CoordinationSystem::totalAllocationsMade = 0;
int CoordinationSystem::nextCaseID = 1001;

/* ============================================================================
 *  INPUT HELPERS - safe numeric input handling (edge case requirement)
 * ============================================================================ */
int readInt(const string& prompt) {
    int value;
    while (true) {
        cout << prompt;
        cin >> value;
        if (cin.fail()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "  [ERROR] Invalid numeric input. Please try again.\n";
            continue;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        return value;
    }
}

string readLine(const string& prompt) {
    string value;
    cout << prompt;
    getline(cin, value);
    return value;
}

ResourceCategory readCategory() {
    cout << "  Select Category -> 1.Ambulance  2.Bed  3.Equipment : ";
    int c;
    cin >> c;
    if (cin.fail()) { cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n'); return ResourceCategory::UNKNOWN; }
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    switch (c) {
        case 1: return ResourceCategory::AMBULANCE;
        case 2: return ResourceCategory::BED;
        case 3: return ResourceCategory::EQUIPMENT;
        default: return ResourceCategory::UNKNOWN;
    }
}

/* ============================================================================
 *  MENU-DRIVEN INTERFACE
 * ============================================================================ */
void printMenu() {
    cout << "\n========================================================\n";
    cout << "       SMART EMERGENCY MEDICAL RESOURCE\n";
    cout << "              COORDINATION SYSTEM\n";
    cout << "========================================================\n";
    cout << " 1. Register Patient\n";
    cout << " 2. Register Emergency Resource\n";
    cout << " 3. Create Emergency Case\n";
    cout << " 4. Calculate / Recalculate Emergency Coordination Score\n";
    cout << " 5. View Pending Emergency Cases\n";
    cout << " 6. Allocate Best Resource\n";
    cout << " 7. View Resource Status\n";
    cout << " 8. Complete Emergency Case (Release Resource)\n";
    cout << " 9. Compare Emergency Cases\n";
    cout << "10. View Resource Utilization\n";
    cout << "11. Generate Coordination Report\n";
    cout << "12. Display System Statistics\n";
    cout << "13. List Patients / Resources\n";
    cout << "14. Exit\n";
    cout << "========================================================\n";
}

int main() {
    CoordinationSystem system;
    bool running = true;

    cout << "Smart Emergency Medical Resource Coordination System - Booting...\n";

    while (running) {
        printMenu();
        int choice = readInt("Enter your choice: ");

        switch (choice) {
            case 1: {
                int id = readInt("  Patient ID: ");
                string name = readLine("  Name: ");
                int age = readInt("  Age: ");
                string contact = readLine("  Contact Number: ");
                system.registerPatient(id, name, age, contact);
                break;
            }
            case 2: {
                int id = readInt("  Resource ID: ");
                ResourceCategory cat = readCategory();
                string zone = readLine("  Zone/Location: ");
                int capLevel = readInt("  Capability Level (1-5): ");
                if (capLevel < 1 || capLevel > 5) {
                    cout << "  [ERROR] Invalid capability level.\n"; break;
                }
                EmergencyResource* r = nullptr;
                if (cat == ResourceCategory::AMBULANCE) {
                    int resp = readInt("  Response Time (minutes): ");
                    r = new Ambulance(id, zone, resp, capLevel);
                } else if (cat == ResourceCategory::BED) {
                    r = new EmergencyBed(id, zone, capLevel);
                } else if (cat == ResourceCategory::EQUIPMENT) {
                    int resp = readInt("  Response Time (minutes): ");
                    string name = readLine("  Equipment Name: ");
                    r = new MedicalEquipment(id, zone, resp, capLevel, name);
                } else {
                    cout << "  [ERROR] Invalid resource type.\n"; break;
                }
                system.registerResource(r);
                break;
            }
            case 3: {
                int pid = readInt("  Patient ID: ");
                int urgency = readInt("  Urgency Level (1-10): ");
                int wait = readInt("  Current Waiting Time (minutes): ");
                ResourceCategory cat = readCategory();
                int capLevel = readInt("  Required Capability Level (1-5): ");
                system.createEmergencyCase(pid, urgency, wait, cat, capLevel);
                break;
            }
            case 4: {
                system.viewPendingCases();
                break;
            }
            case 5: {
                system.viewPendingCases();
                break;
            }
            case 6: {
                int cid = readInt("  Case ID to allocate: ");
                system.allocateBestResource(cid);
                break;
            }
            case 7: {
                system.updateResourceStatus();
                break;
            }
            case 8: {
                int cid = readInt("  Case ID to complete: ");
                int busy = readInt("  Total Busy Minutes for resource: ");
                system.completeEmergencyCase(cid, busy);
                break;
            }
            case 9: {
                int id1 = readInt("  First Case ID: ");
                int id2 = readInt("  Second Case ID: ");
                system.compareCases(id1, id2);
                break;
            }
            case 10: {
                system.viewResourceUtilization();
                break;
            }
            case 11: {
                system.generateCoordinationReport();
                break;
            }
            case 12: {
                system.displaySystemStatistics();
                break;
            }
            case 13: {
                system.listPatients();
                system.listResources();
                break;
            }
            case 14: {
                cout << "Exiting Smart Emergency Medical Resource Coordination System.\n";
                running = false;
                break;
            }
            default: {
                cout << "  [ERROR] Invalid menu choice. Please select 1-14.\n";
                break;
            }
        }
    }
    return 0;
}
