#include <iostream>
#include <string>
using namespace std;

// =====================
// Data Pendukung
// =====================

enum class DeliveryMethod {
    PICKUP,
    COURIER
};

struct Order {
    string customerName;
    string medicineName;
    bool requiresPrescription;
    bool prescriptionUploaded;
    DeliveryMethod deliveryMethod;
    double price;
};

// =====================
// Subsystem 1: Inventory
// =====================

class InventoryService {
public:
    bool checkStock(const string& medicineName) {
        cout << "[Inventory] Mengecek stok obat: " << medicineName << endl;

        // Simulasi: stok selalu tersedia
        return true;
    }

    void lockStock(const string& medicineName) {
        cout << "[Inventory] Stok obat \"" << medicineName
             << "\" dikunci selama 60 menit." << endl;
    }
};

// =====================
// Subsystem 2: Prescription
// =====================

class PrescriptionService {
public:
    bool validatePrescription(const Order& order) {
        if (!order.requiresPrescription) {
            cout << "[Prescription] Obat tidak memerlukan resep dokter." << endl;
            return true;
        }

        cout << "[Prescription] Obat memerlukan resep dokter." << endl;

        if (!order.prescriptionUploaded) {
            cout << "[Prescription] Resep belum diunggah. Pesanan ditolak." << endl;
            return false;
        }

        cout << "[Prescription] Resep berhasil divalidasi oleh apoteker." << endl;
        return true;
    }
};

// =====================
// Subsystem 3: Payment & Escrow
// =====================

class PaymentService {
public:
    bool processEscrowPayment(double amount) {
        cout << "[Payment] Memproses pembayaran sebesar Rp" << amount << endl;
        cout << "[Payment] Dana disimpan sementara di escrow." << endl;

        // Simulasi: pembayaran berhasil
        return true;
    }

    void releaseEscrow() {
        cout << "[Payment] Dana escrow dilepaskan ke pihak apotek." << endl;
    }
};

// =====================
// Subsystem 4: QR Code
// =====================

class QRCodeService {
public:
    string generateQRCode(const string& customerName) {
        string qrCode = "QR-" + customerName + "-ORDER";

        cout << "[QR Code] QR Code dibuat: " << qrCode << endl;

        return qrCode;
    }
};

// =====================
// Subsystem 5: Courier
// =====================

class CourierService {
public:
    void findCourier() {
        cout << "[Courier] Mencari kurir terdekat melalui API Gojek/Grab." << endl;
    }

    void sendDeliveryRequest() {
        cout << "[Courier] Notifikasi pengiriman dikirim ke kurir." << endl;
    }
};

// =====================
// Subsystem 6: Notification
// =====================

class NotificationService {
public:
    void sendNotification(const string& message) {
        cout << "[Notification] " << message << endl;
    }
};

// =====================
// Facade
// =====================

class HealthPointOrderFacade {
private:
    InventoryService inventoryService;
    PrescriptionService prescriptionService;
    PaymentService paymentService;
    QRCodeService qrCodeService;
    CourierService courierService;
    NotificationService notificationService;

public:
    void placeOrder(const Order& order) {
        cout << "\n=== Memulai Proses Pemesanan Health-Point Locator ===\n";

        bool stockAvailable = inventoryService.checkStock(order.medicineName);

        if (!stockAvailable) {
            notificationService.sendNotification("Pesanan gagal. Stok obat tidak tersedia.");
            return;
        }

        bool prescriptionValid = prescriptionService.validatePrescription(order);

        if (!prescriptionValid) {
            notificationService.sendNotification("Pesanan gagal. Resep dokter tidak valid.");
            return;
        }

        bool paymentSuccess = paymentService.processEscrowPayment(order.price);

        if (!paymentSuccess) {
            notificationService.sendNotification("Pesanan gagal. Pembayaran tidak berhasil.");
            return;
        }

        inventoryService.lockStock(order.medicineName);

        if (order.deliveryMethod == DeliveryMethod::PICKUP) {
            string qrCode = qrCodeService.generateQRCode(order.customerName);

            notificationService.sendNotification(
                "Pesanan berhasil. Silakan ambil obat di apotek menggunakan QR Code: " + qrCode
            );
        } else {
            courierService.findCourier();
            courierService.sendDeliveryRequest();

            notificationService.sendNotification(
                "Pesanan berhasil. Kurir sedang diproses untuk pengiriman obat."
            );
        }

        cout << "=== Proses Pemesanan Selesai ===\n";
    }

    void completeOrder() {
        cout << "\n=== Menyelesaikan Pesanan ===\n";

        paymentService.releaseEscrow();
        notificationService.sendNotification("Pesanan selesai. Dana telah diteruskan ke apotek.");

        cout << "=== Pesanan Selesai ===\n";
    }
};

// =====================
// Client
// =====================

int main() {
    HealthPointOrderFacade orderFacade;

    Order pickupOrder;
    pickupOrder.customerName = "Budi";
    pickupOrder.medicineName = "Amoxicillin";
    pickupOrder.requiresPrescription = true;
    pickupOrder.prescriptionUploaded = true;
    pickupOrder.deliveryMethod = DeliveryMethod::PICKUP;
    pickupOrder.price = 45000;

    orderFacade.placeOrder(pickupOrder);
    orderFacade.completeOrder();

    Order courierOrder;
    courierOrder.customerName = "Siti";
    courierOrder.medicineName = "Paracetamol";
    courierOrder.requiresPrescription = false;
    courierOrder.prescriptionUploaded = false;
    courierOrder.deliveryMethod = DeliveryMethod::COURIER;
    courierOrder.price = 20000;

    orderFacade.placeOrder(courierOrder);
    orderFacade.completeOrder();

    return 0;
}